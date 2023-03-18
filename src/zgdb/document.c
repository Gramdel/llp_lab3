#include <malloc.h>
#include <time.h>
#include <string.h>

#include "document.h"
#include "schema.h"
#include "element.h"

document* createDocument() {
    documentSchema* schema = malloc(sizeof(documentSchema));
    if (schema) {
        *schema = (documentSchema) { 0 };
        document* doc = malloc(sizeof(document));
        if (doc) {
            doc->schema = schema;
            return doc;
        }
        free(schema);
    }
    return NULL;
}

void destroyDocument(document* doc) {
    if (doc) {
        if (doc->schema) {
            destroySchema(doc->schema);
        }
        free(doc);
    }
}

bool createRoot(zgdbFile* file, documentSchema* schema) {
    if (file->header.indexOfRoot == DOCUMENT_NOT_EXIST) {
        documentRef* root = writeDocument(file, schema);
        if (root) {
            file->header.indexOfRoot = root->indexNumber;
            free(root);
            return writeHeader(file);
        }
    }
    return false;
}

bool moveFirstDocuments(zgdbFile* file) {
    // Смещаемся к началу документов:
    int64_t newPos;
    int64_t oldPos = (int64_t) (sizeof(zgdbHeader) + sizeof(zgdbIndex) * file->header.indexCount +
                                file->header.firstDocumentOffset);
    /* Перемещаем документы, пока места недостаточно. Изначально доступно file->header.firstDocumentOffset, поскольку
     * перед документами могут быть неиспользуемые байты: */
    int64_t neededSpace = sizeof(zgdbIndex) * ZGDB_DEFAULT_INDEX_CAPACITY;
    int64_t availableSpace = file->header.firstDocumentOffset;
    while (availableSpace < neededSpace) {
        // Считываем заголовок документа:
        fseeko64(file->f, oldPos, SEEK_SET);
        documentHeader header;
        if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
            return false;
        }
        // Считываем индекс, привязанный к документу:
        uint64_t newHeaderSize = 0;
        zgdbIndex index = getIndex(file, header.indexNumber);
        if (index.flag == INDEX_DEAD) {
            // Если наш документ - дырка, удаляем его из списка индексов и делаем INDEX_NEW.
            if (!removeNodeByIndexNumber(&file->list, header.indexNumber) ||
                !updateIndex(file, header.indexNumber, wrap_uint8_t(INDEX_NEW), wrap_int64_t(0))) {
                return false;
            }
            insertNode(&file->list, createNode(0, header.indexNumber));
            oldPos += (int64_t) header.size;
        } else if (index.flag == INDEX_ALIVE) {
            /* Если документ живой, то его нужно переместить.
             * Если есть подходящая дырка, в которую можно переместить документ, то нужно сделать индекс дырки новым
             * (flag = INDEX_NEW, offset = 0), а прошлое смещение дырки записать в индекс переносимого блока.
             * Если подходящих дырок нет (или список пустой), то нужно перемещать документ в конец файла. */
            if (file->list.front && file->list.front->size >= header.size) {
                zgdbIndex gapIndex = getIndex(file, file->list.front->indexNumber);
                if (gapIndex.flag != INDEX_DEAD ||
                    !updateIndex(file, file->list.front->indexNumber, wrap_uint8_t(INDEX_NEW), wrap_int64_t(0))) {
                    return false;
                }
                newPos = gapIndex.offset;
                newHeaderSize = file->list.front->size;
                // Записываем дырку обратно в список, но уже с размером 0:
                listNode* node = popFront(&file->list);
                node->size = 0;
                insertNode(&file->list, node);
                // Если дырка больше, чем надо, записываем TYPE_NOT_EXIST в том месте, где будет заканчиваться документ:
                if (newHeaderSize > header.size) {
                    uint8_t startOfUnusedSpaceMark = TYPE_NOT_EXIST;
                    fseeko64(file->f, newPos + (int64_t) header.size, SEEK_SET);
                    if (!fwrite(&startOfUnusedSpaceMark, sizeof(uint8_t), 1, file->f)) {
                        return false;
                    }
                }
            } else {
                newPos = file->header.fileSize;
                // Обновляем размер файла:
                file->header.fileSize += (int64_t) header.size;
                if (!writeHeader(file)) {
                    return false;
                }
            }
            // Перемещаем документ, обновляем смещение в его индексе:
            if (!updateIndex(file, header.indexNumber, not_present_uint8_t(), wrap_int64_t(newPos)) ||
                !moveData(file, &oldPos, &newPos, header.size)) {
                return false;
            }
        } else {
            return false;
        }
        availableSpace += (int64_t) header.size; // возможно переполнение, если ZGDB_DEFAULT_INDEX_CAPACITY будет слишком большим!
        // Смещаемся к началу нового места документа и обновляем его заголовок, если он был перемещён в дырку:
        if (newHeaderSize) {
            fseeko64(file->f, newPos - (int64_t) header.size, SEEK_SET);
            header.size = newHeaderSize;
            if (!fwrite(&header, sizeof(documentHeader), 1, file->f)) {
                return false;
            }
        }
    }
    // Записываем новые индексы и сохраняем остаток места:
    file->header.firstDocumentOffset = availableSpace % sizeof(zgdbIndex);
    if (!writeNewIndexes(file, availableSpace / sizeof(zgdbIndex)) || !writeHeader(file)) {
        return false;
    }
    return true;
}

documentRef* writeDocument(zgdbFile* file, documentSchema* schema) {
    if (!schema) {
        return NULL;
    }
    int64_t pos = ftello64(file->f); // сохраняем текущую позицию, чтобы вернуться в неё после записи
    documentHeader header;
    header.size = calcDocumentSize(schema);
    header.parentIndexNumber = DOCUMENT_NOT_EXIST; // указывает на то, что родителя нет
    memset(header.schemaName, 0, 13);
    strcpy(header.schemaName, schema->name);

    // Сразу выделяем индексы, если список пустой:
    if (!file->list.front && !moveFirstDocuments(file)) {
        return NULL;
    }
    // Если есть подходящая дырка, то пишем документ туда:
    uint64_t newSize = 0;
    int64_t diff = (int64_t) file->list.front->size - (int64_t) header.size;
    if (diff >= 0) {
        // Считываем индекс дырки и обновляем его (делаем INDEX_ALIVE):
        zgdbIndex index = getIndex(file, file->list.front->indexNumber);
        if (index.flag != INDEX_DEAD ||
            !updateIndex(file, file->list.front->indexNumber, wrap_uint8_t(INDEX_ALIVE), not_present_int64_t())) {
            return NULL;
        }
        // Заполняем заголовок документа:
        header.indexNumber = file->list.front->indexNumber;
        header.id.offset = index.offset;
        // Если дырка больше, чем надо, записываем TYPE_NOT_EXIST в том месте, где будет заканчиваться документ:
        if (diff) {
            uint8_t startOfUnusedSpaceMark = TYPE_NOT_EXIST;
            fseeko64(file->f, index.offset + (int64_t) header.size, SEEK_SET);
            if (!fwrite(&startOfUnusedSpaceMark, sizeof(uint8_t), 1, file->f)) {
                return NULL;
            }
            newSize = file->list.front->size;
        }
        free(popFront(&file->list));
    } else {
        // В любом случае будем писать в конец файла, но, возможно, надо выделить новые индексы. Затем обновляем индекс дырки:
        if (file->list.back->size != 0 && !moveFirstDocuments(file) ||
            !updateIndex(file, file->list.back->indexNumber, wrap_uint8_t(INDEX_ALIVE),
                         wrap_int64_t(file->header.fileSize))) {
            return NULL;
        }
        // Заполняем заголовок документа:
        header.indexNumber = file->list.back->indexNumber;
        header.id.offset = file->header.fileSize;
        free(popBack(&file->list));
        // Обновляем размер файла
        file->header.fileSize += (int64_t) header.size;
        if (!writeHeader(file)) {
            return NULL;
        }
    }

    // Пропускаем заголовок и записываем сначала основную часть документа:
    uint64_t bytesLeft = header.size;
    fseeko64(file->f, header.id.offset + (int64_t) sizeof(documentHeader), SEEK_SET);
    for (uint64_t i = 0; i < schema->length; i++) {
        bytesLeft -= writeElement(file, schema->elements[i], header.indexNumber);
    }
    // Обновляем размер документа (если надо) и записываем время создания документа в заголовок:
    if (newSize) {
        header.size = newSize;
    }
    header.id.timestamp = (uint32_t) time(NULL);
    // Перемещаемся к началу и записываем заголовок:
    fseeko64(file->f, header.id.offset, SEEK_SET);
    bytesLeft -= fwrite(&header, sizeof(documentHeader), 1, file->f) * sizeof(documentHeader);
    documentRef* ref = NULL;
    if (!bytesLeft) {
        ref = malloc(sizeof(documentRef));
        if (ref) {
            ref->indexNumber = header.indexNumber;
        }
    }
    fseeko64(file->f, pos, SEEK_SET);
    return ref;
}

// TODO: free(el)?
document* readDocument(zgdbFile* file, uint64_t indexNumber) {
    zgdbIndex index = getIndex(file, indexNumber);
    if (index.flag == INDEX_ALIVE) {
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
        documentHeader header;
        if (fread(&header, sizeof(documentHeader), 1, file->f)) {
            document* doc = createDocument();
            if (doc) {
                doc->header = header;
                strcpy(doc->schema->name, doc->header.schemaName);
                uint64_t bytesRead = sizeof(documentHeader);
                while (bytesRead < header.size) {
                    element* el = malloc(sizeof(element));
                    if (!el) {
                        destroyDocument(doc);
                        return NULL;
                    }
                    uint64_t tmp = readElement(file, el, false);
                    if (!tmp) {
                        destroyDocument(doc);
                        return NULL;
                    }
                    // Если элемент не существует, то выходим из цикла. Иначе - пробуем добавить элемент в схему:
                    if (el->type == TYPE_NOT_EXIST) {
                        bytesRead = header.size;
                    } else {
                        bytesRead += tmp;
                        if (!addElementToSchema(doc->schema, el)) {
                            destroyDocument(doc);
                            return NULL;
                        }
                    }
                }
                return doc;
            }
        }
    }
    return NULL;
}

indexFlag removeEmbeddedDocument(zgdbFile* file, uint64_t childIndexNumber, uint64_t parentIndexNumber) {
    zgdbIndex index = getIndex(file, childIndexNumber);
    if (index.flag == INDEX_ALIVE) {
        // Считываем хедер документа:
        fseeko64(file->f, index.offset, SEEK_SET);
        documentHeader header;
        if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
            return INDEX_NOT_EXIST;
        }

        /* Если номер индекса документа-родителя не совпадает с header.parentIndexNumber в документе-ребёнке, и при этом
         * инициатор удаления - документ, а не пользователь (т.е. номер индекса документа-родителя отличен от DOCUMENT_NOT_EXIST),
         * то нужно отменить удаление: */
        if (parentIndexNumber != header.parentIndexNumber && parentIndexNumber != DOCUMENT_NOT_EXIST) {
            return index.flag;
        }

        // Если header.parentIndexNumber в документе-ребёнке не равен DOCUMENT_NOT_EXIST, то обновляем информацию в родителе:
        if (header.parentIndexNumber != DOCUMENT_NOT_EXIST) {
            // Обновляем
        }

        // Изменяем флаг индекса документа и добавляем дырку в список:
        if (!updateIndex(file, childIndexNumber, wrap_uint8_t(INDEX_DEAD), not_present_int64_t())) {
            return INDEX_NOT_EXIST;
        }
        insertNode(&file->list, createNode(header.size, childIndexNumber));

        // Удаляем детей документа:
        for (uint64_t i = 0; i < file->header.indexCount; i++) {
            if (i != childIndexNumber) {
                if (removeEmbeddedDocument(file, i, childIndexNumber) == INDEX_NOT_EXIST) {
                    return INDEX_NOT_EXIST;
                }
            }
        }
    }
    return index.flag;
}

bool removeDocument(zgdbFile* file, documentRef* ref) {
    bool result = false;
    if (ref && ref->indexNumber != DOCUMENT_NOT_EXIST) {
        result = removeEmbeddedDocument(file, ref->indexNumber, DOCUMENT_NOT_EXIST) != INDEX_NOT_EXIST;
        ref->indexNumber = DOCUMENT_NOT_EXIST;
    }
    return result;
}

void printDocument(zgdbFile* file, document* doc) {
    if (doc) {
        printf("%s#%08X%016X {\n", doc->header.schemaName, doc->header.id.timestamp, doc->header.id.offset);
        for (uint64_t i = 0; i < doc->schema->length; i++) {
            printf("\t");
            printElement(file, doc->schema->elements[i]);
        }
        printf("}\n");
    }
}

bool updateDocument(zgdbFile* file, uint64_t indexNumber, documentSchema* newValues) {
    // Если обновлять документ не надо (newValues == null), то возвращаем true:
    printf("%d\n", indexNumber);
    if (!newValues) {
        return true;
    }
    zgdbIndex index = getIndex(file, indexNumber);
    if (index.flag == INDEX_ALIVE) {
        fseeko64(file->f, index.offset, SEEK_SET);
        documentHeader header;
        if (fread(&header, sizeof(documentHeader), 1, file->f)) {
            uint64_t bytesLeft = header.size - sizeof(documentHeader);
            while (bytesLeft > 0) {
                element oldElement;
                uint64_t tmp = readElement(file, &oldElement, true);
                if (!tmp) {
                    return false;
                }
                // Если элемент не существует, то выходим из цикла. Иначе - обновляем элемент:
                if (oldElement.type == TYPE_NOT_EXIST) {
                    bytesLeft = 0;
                } else {
                    bytesLeft -= tmp;
                    element* newElement = getElementFromSchema(newValues, oldElement.key);
                    if (newElement) {
                        fseeko64(file->f, (int64_t) -tmp, SEEK_CUR);
                        if (newElement->type == TYPE_STRING) {
                            updateStringElement(file, &index, &header, &oldElement, newElement);
                        } else if (newElement->type == TYPE_EMBEDDED_DOCUMENT) {

                        } else if (!writeElement(file, newElement, indexNumber)) {
                            return false;
                        }
                        fseeko64(file->f, 0, SEEK_CUR); // этот вызов нужен для того, чтобы можно было сделать fread
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool remDocument(zgdbFile* file, uint64_t indexNumber, documentSchema* newValues) {
    return true;
}

void destroyDocumentRef(documentRef* ref) {
    if (ref) {
        free(ref);
    }
}

element* getElementFromDocument(document* doc, const char* key) {
    return doc ? getElementFromSchema(doc->schema, key) : NULL;
}

documentSchema* getSchemaFromDocument(document* doc) {
    return doc ? doc->schema : NULL;
}