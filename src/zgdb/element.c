#include <malloc.h>
#include <string.h>

#include "format.h"
#include "document.h"
#include "element.h"

uint64_t writeElement(zgdbFile* file, element* el, uint64_t parentIndexNumber) {
    documentRef* ref; // переменная для ссылки на вложенный документ
    uint64_t bytesWritten = 0;
    bytesWritten += fwrite(&el->type, sizeof(uint8_t), 1, file->f);
    bytesWritten += fwrite(el->key, sizeof(char), 13, file->f) * sizeof(char);
    switch (el->type) {
        case TYPE_INT:
            bytesWritten += fwrite(&el->integerValue, sizeof(int32_t), 1, file->f) * sizeof(int32_t);
            break;
        case TYPE_DOUBLE:
            bytesWritten += fwrite(&el->doubleValue, sizeof(double), 1, file->f) * sizeof(double);
            break;
        case TYPE_BOOLEAN:
            bytesWritten += fwrite(&el->booleanValue, sizeof(uint8_t), 1, file->f);
            break;
        case TYPE_STRING:
            bytesWritten += fwrite(&el->stringValue.size, sizeof(uint32_t), 1, file->f) * sizeof(uint32_t);
            bytesWritten += fwrite(el->stringValue.data, sizeof(char), el->stringValue.size, file->f);
            break;
        case TYPE_EMBEDDED_DOCUMENT:
            ref = writeDocument(file, el->schemaValue);
            if (!ref) {
                return 0;
            }

            uint64_t tmp = ref->indexNumber; // для битового поля
            bytesWritten += fwrite(&tmp, 5, 1, file->f) * 5; // uint64_t : 40 == 5 байт
            int64_t pos = ftello64(file->f); // сохраняем позицию до записи заголовков вложенного документа

            // Записываем в хедер вложенного документа информацию об этом (создаваемом) документе:
            zgdbIndex index = getIndex(file, tmp);
            if (index.flag != INDEX_ALIVE) {
                return 0;
            } else {
                /* Спускаемся к хедеру ребёнка и пропускаем размер, номер индекса.
                 * Считываем parentIndexNumber, если он не равен DOCUMENT_NOT_EXIST, то завершаем выполнение: */
                fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                if (!fread(&tmp, 5, 1, file->f) || tmp != DOCUMENT_NOT_EXIST) {
                    return 0;
                }
                // Снова спускаемся к хедеру. Записываем parentIndexNumber:
                fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                if (!fwrite(&parentIndexNumber, 5, 1, file->f)) {
                    return 0;
                }
            }
            fseeko64(file->f, pos, SEEK_SET); // восстанавливаем позицию
            break;
    }
    return bytesWritten;
}

element* readElement(zgdbFile* file, char* neededKey, documentRef* ref) {
    if (ref && ref->indexNumber != DOCUMENT_NOT_EXIST) {
        uint64_t tmp = 0; // для битового поля (documentValue)
        element* el = malloc(sizeof(element));
        if (el) {
            strcpy(el->key, neededKey);
            el->type = navigateToElement(file, neededKey, ref->indexNumber);
            switch (el->type) {
                case TYPE_NOT_EXIST:
                    break;
                case TYPE_INT:
                    if (fread(&el->integerValue, sizeof(int32_t), 1, file->f)) {
                        return el;
                    }
                    break;
                case TYPE_DOUBLE:
                    if (fread(&el->doubleValue, sizeof(double), 1, file->f)) {
                        return el;
                    }
                    break;
                case TYPE_BOOLEAN:
                    if (fread(&el->booleanValue, sizeof(uint8_t), 1, file->f)) {
                        return el;
                    }
                    break;
                case TYPE_STRING:
                    if (fread(&el->stringValue.size, sizeof(uint32_t), 1, file->f)) {
                        el->stringValue.data = malloc(sizeof(char) * el->stringValue.size);
                        if (el->stringValue.data) {
                            if (fread(el->stringValue.data, sizeof(char), el->stringValue.size, file->f) ==
                                el->stringValue.size) {
                                return el;
                            }
                            free(el->stringValue.data);
                        }
                    }
                    break;
                case TYPE_EMBEDDED_DOCUMENT:
                    el->documentValue = malloc(sizeof(documentRef));
                    if (el->documentValue) {
                        if (fread(&tmp, 5, 1, file->f)) {
                            el->documentValue->indexNumber = tmp;
                            return el;
                        }
                        free(el->documentValue);
                    }
                    break;
            }
            free(el);
        }
    }
    return NULL;
}

elementType navigateToElement(zgdbFile* file, char* neededKey, uint64_t i) {
    zgdbIndex index = getIndex(file, i);
    if (index.flag == INDEX_ALIVE) {
        fseeko64(file->f, index.offset, SEEK_SET); // спуск в документ по смещению
        documentHeader header;
        if (fread(&header, sizeof(documentHeader), 1, file->f)) {
            uint64_t bytesRead = sizeof(documentHeader);
            element el;
            while (bytesRead < header.size) {
                if (!fread(&el.type, sizeof(uint8_t), 1, file->f) || fread(&el.key, sizeof(char), 13, file->f) != 13) {
                    goto exit;
                } else if (strcmp(el.key, neededKey) == 0) {
                    fseeko64(file->f, 0, SEEK_CUR); // без этого вызова любой fwrite будет пытаться записать в eof!
                    return el.type;
                } else {
                    bytesRead += sizeof(uint8_t) + sizeof(char) * 13;
                    switch (el.type) {
                        case TYPE_NOT_EXIST:
                            bytesRead = header.size;
                            break;
                        case TYPE_INT:
                            if (fseeko64(file->f, sizeof(int32_t), SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(int32_t);
                            break;
                        case TYPE_DOUBLE:
                            if (fseeko64(file->f, sizeof(double), SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(double);
                            break;
                        case TYPE_BOOLEAN:
                            if (fseeko64(file->f, sizeof(uint8_t), SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(uint8_t);
                            break;
                        case TYPE_STRING:
                            if (!fread(&el.stringValue.size, sizeof(uint32_t), 1, file->f) ||
                                fseeko64(file->f, el.stringValue.size, SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += sizeof(uint32_t) + sizeof(char) * el.stringValue.size;
                            break;
                        case TYPE_EMBEDDED_DOCUMENT:
                            if (fseeko64(file->f, 5, SEEK_CUR) != 0) {
                                goto exit;
                            }
                            bytesRead += 5;
                            break;
                    }
                }
            }
        }
    }
    exit:
    return TYPE_NOT_EXIST;
}

void destroyElement(element* el) {
    if (el) {
        if (el->type == TYPE_STRING) {
            if (el->stringValue.data) {
                free(el->stringValue.data);
            }
        } else if (el->type == TYPE_EMBEDDED_DOCUMENT) {
            if (el->documentValue) {
                free(el->documentValue);
            }
        }
        free(el);
    }
}

void printElementOfEmbeddedDocument(zgdbFile* file, element* el, uint64_t nestingLevel) {
    if (el) {
        switch (el->type) {
            case TYPE_NOT_EXIST:
                printf("%*sElement doesn't exist or there is some unused space in the document!\n", nestingLevel * 2,
                       "");
                break;
            case TYPE_INT:
                printf("%*skey: \"%s\", integerValue: %d\n", nestingLevel * 2, "", el->key, el->integerValue);
                break;
            case TYPE_DOUBLE:
                printf("%*skey: \"%s\", doubleValue: %f\n", nestingLevel * 2, "", el->key, el->doubleValue);
                break;
            case TYPE_BOOLEAN:
                printf("%*skey: \"%s\", booleanValue: %s\n", nestingLevel * 2, "", el->key,
                       el->booleanValue ? "true" : "false");
                break;
            case TYPE_STRING:
                printf("%*skey: \"%s\", stringValue: \"%s\"\n", nestingLevel * 2, "", el->key, el->stringValue.data);
                break;
            case TYPE_EMBEDDED_DOCUMENT:
                printf("%*skey: \"%s\", documentValue:\n", nestingLevel * 2, "", el->key);
                printEmbeddedDocument(file, el->documentValue->indexNumber, nestingLevel + 1);
                break;
        }
    } else {
        printf("%*sElement doesn't exist!\n", nestingLevel * 2, "");
    }
}

void printElement(zgdbFile* file, element* el) {
    printElementOfEmbeddedDocument(file, el, 0);
}

// TODO: перевести все геттеры на работу с указателями
elementType getTypeOfElement(element el) {
    return el.type;
}

int32_t getIntegerValue(element el) {
    return el.integerValue;
}

double getDoubleValue(element el) {
    return el.doubleValue;
}

uint8_t getBooleanValue(element el) {
    return el.booleanValue;
}

char* getStringValue(element el) {
    return el.stringValue.data;
}

documentRef* getDocumentValue(element el) {
    return el.documentValue;
}

bool updateIntegerValue(zgdbFile* file, char* neededKey, int32_t value, documentRef* ref) {
    if (!ref || ref->indexNumber == DOCUMENT_NOT_EXIST) {
        return false;
    }
    return navigateToElement(file, neededKey, ref->indexNumber) == TYPE_INT &&
           fwrite(&value, sizeof(int32_t), 1, file->f);
}

bool updateDoubleValue(zgdbFile* file, char* neededKey, double value, documentRef* ref) {
    if (!ref || ref->indexNumber == DOCUMENT_NOT_EXIST) {
        return false;
    }
    return navigateToElement(file, neededKey, ref->indexNumber) == TYPE_DOUBLE &&
           fwrite(&value, sizeof(double), 1, file->f);
}

bool updateBooleanValue(zgdbFile* file, char* neededKey, uint8_t value, documentRef* ref) {
    if (!ref || ref->indexNumber == DOCUMENT_NOT_EXIST) {
        return false;
    }
    return navigateToElement(file, neededKey, ref->indexNumber) == TYPE_BOOLEAN &&
           fwrite(&value, sizeof(uint8_t), 1, file->f);
}

bool updateStringValue(zgdbFile* file, char* neededKey, char* value, documentRef* ref) {
    // Проверяем ссылку на документ:
    if (!ref || ref->indexNumber == DOCUMENT_NOT_EXIST) {
        return false;
    }
    // Получаем индекс документа:
    zgdbIndex index = getIndex(file, ref->indexNumber);
    if (index.flag != INDEX_ALIVE) {
        return false;
    }
    // Перемещаемся к началу обновляемого документа и считываем его заголовок:
    fseeko64(file->f, index.offset, SEEK_SET);
    documentHeader header;
    if (!fread(&header, sizeof(documentHeader), 1, file->f)) {
        return false;
    }
    // Спускаемся к нужному элементу по ключу, сохраняем относительное смещение:
    if (navigateToElement(file, neededKey, ref->indexNumber) != TYPE_STRING) {
        return false;
    }
    int64_t offsetOfValue = ftello64(file->f) - index.offset;
    // Считываем предыдущий размер строки:
    uint32_t newSize = strlen(value) + 1;
    uint32_t oldSize;
    if (!fread(&oldSize, sizeof(uint32_t), 1, file->f)) {
        return false;
    }

    // Если строка стала больше, то ей нужно найти новое место:
    int64_t delta = (int64_t) newSize - (int64_t) oldSize; // изменение размера строки
    uint64_t newHeaderSize = 0;
    if (delta > 0) {
        // Если документ не в конце файла, ищем дырку и переносим его:
        if (index.offset + header.size != file->header.fileSize) {
            int64_t oldPos = index.offset;
            int64_t newPos;
            documentHeader gapHeader = header;
            int64_t diff = file->list.front ? (int64_t) file->list.front->size - (int64_t) header.size - delta : -1;
            if (diff >= 0) {
                // Считываем индекс дырки и обновляем в нём смещение:
                zgdbIndex gapIndex = getIndex(file, file->list.front->indexNumber);
                if (gapIndex.flag != INDEX_DEAD ||
                    !updateIndex(file, file->list.front->indexNumber, not_present_uint8_t(),
                                 wrap_int64_t(index.offset))) {
                    return false;
                }
                gapHeader.indexNumber = file->list.front->indexNumber; // записываем в хедер для будущей дырки номер индекса текущей
                newPos = index.offset = gapIndex.offset;
                newHeaderSize = file->list.front->size;
                listNode* node = popFront(&file->list);
                node->size = header.size;
                insertNode(&file->list, node);
                // Если дырка больше, чем надо, записываем TYPE_NOT_EXIST в том месте, где будет заканчиваться документ:
                if (diff) {
                    uint8_t startOfUnusedSpaceMark = TYPE_NOT_EXIST;
                    fseeko64(file->f, newPos + (int64_t) header.size, SEEK_SET);
                    if (!fwrite(&startOfUnusedSpaceMark, sizeof(uint8_t), 1, file->f)) {
                        return false;
                    }
                }
            } else {
                // На предыдущем месте образуется дырка, следовательно, нужны индексы:
                if ((!file->list.back || file->list.back->size) && !moveFirstDocuments(file)) {
                    return false;
                }
                // Заново считываем индекс документа, поскольку он мог быть перемещён:
                index = getIndex(file, ref->indexNumber);
                if (index.flag != INDEX_ALIVE) {
                    return false;
                }
                // Если документ не переместился в конец, нужно его туда переместить:
                if (index.offset + header.size != file->header.fileSize) {
                    // Считываем INDEX_NEW индекс, делаем его INDEX_DEAD и записываем в него текущее смещение документа:
                    zgdbIndex gapIndex = getIndex(file, file->list.back->indexNumber);
                    if (gapIndex.flag != INDEX_NEW ||
                        !updateIndex(file, file->list.back->indexNumber, wrap_uint8_t(INDEX_DEAD),
                                     wrap_int64_t(index.offset))) {
                        return false;
                    }
                    gapHeader.indexNumber = file->list.back->indexNumber; // записываем в хедер для будущей дырки номер индекса текущей
                    listNode* node = popBack(&file->list);
                    node->size = header.size;
                    insertNode(&file->list, node);
                    oldPos = index.offset;
                    newPos = index.offset = file->header.fileSize;
                    file->header.fileSize += (int64_t) header.size + delta;
                } else {
                    newPos = oldPos; // условие для того, чтобы не перемещать документ
                    file->header.fileSize += delta;
                }
                // Обновляем fileSize:
                if (!writeHeader(file)) {
                    return false;
                }
            }
            // Перемещаем документ, если он не оказался в конце файла, обновляем смещение в его индексе и записываем на его месте хедер дырки:
            if (newPos != oldPos) {
                if (!updateIndex(file, header.indexNumber, not_present_uint8_t(), wrap_int64_t(newPos)) ||
                    !moveData(file, &oldPos, &newPos, header.size)) {
                    return false;
                }
                fseeko64(file->f, oldPos - (int64_t) header.size, SEEK_SET);
                if (!fwrite(&gapHeader, sizeof(documentHeader), 1, file->f)) {
                    return false;
                }
            }
        } else {
            file->header.fileSize += delta;
            if (!writeHeader(file)) {
                return false;
            }
        }
    }

    // Перемещаем кусок документа после строки, чтобы не перекрыть его новой строкой или чтобы не было дырок:
    int64_t oldPos = index.offset + offsetOfValue + (int64_t) sizeof(uint32_t) + oldSize;
    int64_t newPos = oldPos + delta;
    if (!moveData(file, &oldPos, &newPos, index.offset + header.size - oldPos)) {
        return false;
    }
    // Возвращаемся к началу value, перезаписываем размер строки и саму строку:
    fseeko64(file->f, index.offset + offsetOfValue, SEEK_SET);
    if (!fwrite(&newSize, sizeof(uint32_t), 1, file->f) || fwrite(value, sizeof(char), newSize, file->f) != newSize) {
        return false;
    }
    // Перезаписываем размер документа:
    header.size = newHeaderSize ? newHeaderSize : header.size + delta;
    fseeko64(file->f, index.offset, SEEK_SET);
    if (!fwrite(&header, sizeof(documentHeader), 1, file->f)) {
        return false;
    }
    return true;
}

bool updateDocumentValue(zgdbFile* file, char* neededKey, documentSchema* value, documentRef* ref) {
    if (!ref || ref->indexNumber == DOCUMENT_NOT_EXIST) {
        return false;
    }
    // Спускаемся к нужному элементу по ключу:
    if (navigateToElement(file, neededKey, ref->indexNumber) == TYPE_EMBEDDED_DOCUMENT) {
        // Считываем предыдущее значение поля и удаляем документ с таким индексом, если он есть:
        int64_t pos = ftello64(file->f);
        uint64_t oldChildIndexNumber = 0;
        if (fread(&oldChildIndexNumber, 5, 1, file->f)) {
            if (oldChildIndexNumber == DOCUMENT_NOT_EXIST ||
                removeEmbeddedDocument(file, oldChildIndexNumber, ref->indexNumber) == INDEX_ALIVE) {
                // Записываем нового ребёнка в файл:
                documentRef* newValueRef = writeDocument(file, value);
                if (newValueRef) {
                    zgdbIndex index = getIndex(file, newValueRef->indexNumber);
                    if (index.flag == INDEX_ALIVE) {
                        /* Спускаемся к хедеру ребёнка и пропускаем размер, номер индекса.
                         * Записываем parentIndexNumber: */
                        uint64_t tmp = ref->indexNumber;
                        fseeko64(file->f, index.offset + 5 + 5, SEEK_SET);
                        if (fwrite(&tmp, 5, 1, file->f)) {
                            // Возвращаемся к обновляемому полю и записываем новое значение:
                            tmp = newValueRef->indexNumber;
                            fseeko64(file->f, pos, SEEK_SET);
                            return fwrite(&tmp, 5, 1, file->f);
                        }
                    }
                }
            }
        }
    }
    return false;
}