#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "student.h"

// 내부 함수: 헤더에서 총 레코드 수 읽기
static int getRecordCount(FILE *fp) {
    int count = 0;
    fseek(fp, 0, SEEK_SET);
    fread(&count, sizeof(int), 1, fp);
    return count;
}

// 내부 함수: 헤더에 총 레코드 수 쓰기
static void setRecordCount(FILE *fp, int count) {
    fseek(fp, 0, SEEK_SET);
    fwrite(&count, sizeof(int), 1, fp);
    int reserved = 0;
    fwrite(&reserved, sizeof(int), 1, fp);
    fflush(fp);
}

// 레코드 읽기
int readRecord(FILE *fp, STUDENT *s, int rrn) {
    if (fp == NULL || s == NULL) return 0;

    int totalRecords = getRecordCount(fp);
    if (rrn < 0 || rrn >= totalRecords) return 0;

    char recordbuf[RECORD_SIZE + 1];
    long offset = HEADER_SIZE + (long)rrn * RECORD_SIZE;
    fseek(fp, offset, SEEK_SET);
    size_t n = fread(recordbuf, 1, RECORD_SIZE, fp);
    if (n != RECORD_SIZE) {
        perror("fread error in readRecord");
        return 0;
    }

    recordbuf[RECORD_SIZE] = '\0';
    unpack(recordbuf, s);
    return 1;
}

// 레코드 언팩 (필드별 공백 제거 포함)
void unpack(const char *recordbuf, STUDENT *s) {
    char field[5][31];
    char *token;
    int idx = 0;
    char temp[RECORD_SIZE + 1];
    strncpy(temp, recordbuf, RECORD_SIZE);
    temp[RECORD_SIZE] = '\0';

    token = strtok(temp, "#");
    while (token != NULL && idx < 5) {
        strncpy(field[idx], token, sizeof(field[idx]) - 1);
        field[idx][sizeof(field[idx]) - 1] = '\0';

        int len = strlen(field[idx]);
        while (len > 0 && field[idx][len - 1] == ' ') {
            field[idx][--len] = '\0';
        }

        idx++;
        token = strtok(NULL, "#");
    }

    strncpy(s->sid, field[0], sizeof(s->sid) - 1);
    s->sid[sizeof(s->sid) - 1] = '\0';
    strncpy(s->name, field[1], sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
    strncpy(s->dept, field[2], sizeof(s->dept) - 1);
    s->dept[sizeof(s->dept) - 1] = '\0';
    strncpy(s->addr, field[3], sizeof(s->addr) - 1);
    s->addr[sizeof(s->addr) - 1] = '\0';
    strncpy(s->email, field[4], sizeof(s->email) - 1);
    s->email[sizeof(s->email) - 1] = '\0';
}

// 레코드 패킹 (고정 길이 필드 + 구분자 '#')
void pack(char *recordbuf, const STUDENT *s) {
    const int sizes[5] = {8, 10, 12, 30, 20};
    char fields[5][31];

    snprintf(fields[0], sizeof(fields[0]), "%-8s", s->sid);
    snprintf(fields[1], sizeof(fields[1]), "%-10s", s->name);
    snprintf(fields[2], sizeof(fields[2]), "%-12s", s->dept);
    snprintf(fields[3], sizeof(fields[3]), "%-30s", s->addr);
    snprintf(fields[4], sizeof(fields[4]), "%-20s", s->email);

    int pos = 0;
    for (int i = 0; i < 5; i++) {
        strncpy(recordbuf + pos, fields[i], sizes[i]);
        pos += sizes[i];
        recordbuf[pos++] = '#';
    }
    recordbuf[pos] = '\0';
}

// 레코드 쓰기
int writeRecord(FILE *fp, const STUDENT *s, int rrn) {
    if (fp == NULL || s == NULL) return 0;

    char recordbuf[RECORD_SIZE + 1];
    pack(recordbuf, s);

    long offset = HEADER_SIZE + (long)rrn * RECORD_SIZE;
    fseek(fp, offset, SEEK_SET);
    size_t n = fwrite(recordbuf, 1, RECORD_SIZE, fp);
    fflush(fp);

    return (n == RECORD_SIZE) ? 1 : 0;
}

// 레코드 추가
int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email) {
    if (fp == NULL) return 0;

    int totalRecords = getRecordCount(fp);

    STUDENT s;
    strncpy(s.sid, sid, sizeof(s.sid) - 1); s.sid[sizeof(s.sid) - 1] = '\0';
    strncpy(s.name, name, sizeof(s.name) - 1); s.name[sizeof(s.name) - 1] = '\0';
    strncpy(s.dept, dept, sizeof(s.dept) - 1); s.dept[sizeof(s.dept) - 1] = '\0';
    strncpy(s.addr, addr, sizeof(s.addr) - 1); s.addr[sizeof(s.addr) - 1] = '\0';
    strncpy(s.email, email, sizeof(s.email) - 1); s.email[sizeof(s.email) - 1] = '\0';

    if (!writeRecord(fp, &s, totalRecords)) return 0;

    setRecordCount(fp, totalRecords + 1);
    return 1;
}

// 검색 기능 (SID, NAME, DEPT만 검색 가능)
void search(FILE *fp, enum FIELD f, char *keyval) {
    if (fp == NULL || keyval == NULL) return;

    int totalRecords = getRecordCount(fp);
    if (totalRecords == 0) {
        printf("#records = 0\n");
        return;
    }

    STUDENT *results[totalRecords];
    int foundCount = 0;
    STUDENT temp;

    for (int i = 0; i < totalRecords; i++) {
        if (!readRecord(fp, &temp, i)) continue;

        int match = 0;
        switch (f) {
            case SID:
                if (strcmp(temp.sid, keyval) == 0) match = 1;
                break;
            case NAME:
                if (strcmp(temp.name, keyval) == 0) match = 1;
                break;
            case DEPT:
                if (strcmp(temp.dept, keyval) == 0) match = 1;
                break;
            default:
                break;
        }

        if (match) {
            results[foundCount] = malloc(sizeof(STUDENT));
            if (results[foundCount] == NULL) {
                perror("Memory allocation error");
                for (int k = 0; k < foundCount; k++) free(results[k]);
                return;
            }
            *results[foundCount] = temp;
            foundCount++;
        }
    }

    print((const STUDENT **)results, foundCount);

    for (int i = 0; i < foundCount; i++) {
        free(results[i]);
    }
}

// 필드명 -> enum FIELD 변환 (SID, NAME, DEPT만 허용)
enum FIELD getFieldID(char *fieldname) {
    if (strcmp(fieldname, "SID") == 0) return SID;
    else if (strcmp(fieldname, "NAME") == 0) return NAME;
    else if (strcmp(fieldname, "DEPT") == 0) return DEPT;
    else return -1;
}

// 출력 함수 (명세서 요구)
void print(const STUDENT *s[], int n) {
    printf("#records = %d\n", n);
    for (int i = 0; i < n; i++) {
        printf("%s#%s#%s#%s#%s#\n",
               s[i]->sid, s[i]->name, s[i]->dept, s[i]->addr, s[i]->email);
    }
}

// main 함수: 명세서대로 구현
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Append: %s -a record_file \"sid\" \"name\" \"dept\" \"addr\" \"email\"\n", argv[0]);
        fprintf(stderr, "  Search: %s -s record_file \"FIELD=keyvalue\"\n", argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *filename = argv[2];

    FILE *fp = fopen(filename, "r+b");
    if (fp == NULL) {
        fp = fopen(filename, "w+b");
        if (fp == NULL) {
            perror("File open error");
            return 1;
        }
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, fp);
        fwrite(&zero, sizeof(int), 1, fp);
        fflush(fp);
    }

    if (strcmp(mode, "-a") == 0) {
        if (argc != 8) {
            fprintf(stderr, "Append mode requires 5 fields\n");
            fclose(fp);
            return 1;
        }
        if (!append(fp, argv[3], argv[4], argv[5], argv[6], argv[7])) {
            fprintf(stderr, "Append failed\n");
            fclose(fp);
            return 1;
        }
        // append 모드에서는 출력 없음
    } else if (strcmp(mode, "-s") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Search mode requires 1 field condition\n");
            fclose(fp);
            return 1;
        }
        char *condition = argv[3];
        char *equalSign = strchr(condition, '=');
        if (equalSign == NULL) {
            fprintf(stderr, "Invalid search condition format\n");
            fclose(fp);
            return 1;
        }
        *equalSign = '\0';
        char *fieldname = condition;
        char *keyval = equalSign + 1;

        enum FIELD field = getFieldID(fieldname);
        if (field == -1) {
            fprintf(stderr, "Error: Search key must be SID, NAME, or DEPT.\n");
            fclose(fp);
            return 1;
        }
        search(fp, field, keyval);
    } else {
        fprintf(stderr, "Invalid mode\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}
