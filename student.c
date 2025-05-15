#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "student.h"

// 내부 함수: 헤더에서 총 레코드 수 읽기 (C library 사용)
static int getRecordCount(FILE *fp) {
    int count = 0;
    fseek(fp, 0, SEEK_SET);
    fread(&count, sizeof(int), 1, fp);
    return count;
}

// 내부 함수: 헤더에 총 레코드 수 쓰기 (C library 사용)
static void setRecordCount(FILE *fp, int count) {
    fseek(fp, 0, SEEK_SET);
    fwrite(&count, sizeof(int), 1, fp);
    // 예약 공간 4바이트는 0으로 초기화
    int reserved = 0;
    fwrite(&reserved, sizeof(int), 1, fp);
    fflush(fp);
}

// 함수 readRecord: 학생 레코드 파일에서 주어진 rrn에 해당하는 레코드를 읽기 (C library 사용)
int readRecord(FILE *fp, STUDENT *s, int rrn) {
    if (fp == NULL || s == NULL) return 0;

    int totalRecords = getRecordCount(fp);
    if (rrn < 0 || rrn >= totalRecords) return 0;

    char recordbuf[RECORD_SIZE + 1];
    long offset = HEADER_SIZE + (long)rrn * RECORD_SIZE;
    fseek(fp, offset, SEEK_SET);
    size_t n = fread(recordbuf, 1, RECORD_SIZE, fp);
    if (n != RECORD_SIZE) return 0;

    recordbuf[RECORD_SIZE] = '\0'; // null terminate
    unpack(recordbuf, s);
    return 1;
}

// 함수 unpack: recordbuf에 저장된 레코드에서 각 필드 추출
void unpack(const char *recordbuf, STUDENT *s) {
    char field[5][31]; // 각 필드를 저장할 임시 배열
    char *token;
    int idx = 0;
    char temp[RECORD_SIZE + 1];
    strncpy(temp, recordbuf, RECORD_SIZE);
    temp[RECORD_SIZE] = '\0';

    token = strtok(temp, "#");
    while (token != NULL && idx < 5) {
        // 1) 토큰 복사
        strncpy(field[idx], token, sizeof(field[idx]) - 1);
        field[idx][sizeof(field[idx]) - 1] = '\0';

        // 2) 뒤쪽 공백(trim) 제거
        int len = strlen(field[idx]);
        while (len > 0 && field[idx][len-1] == ' ') {
            field[idx][--len] = '\0';
        }

        idx++;
        token = strtok(NULL, "#");
    }

    // 필드 값을 STUDENT 구조체에 복사
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

// 함수 writeRecord: 학생 레코드 파일에 주어진 rrn에 해당하는 위치에 레코드 저장 (C library 사용)
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

// 함수 pack: 학생 레코드 구조체를 recordbuf로 변환
void pack(char *recordbuf, const STUDENT *s) {
    // 각 필드별 고정길이
    const int sizes[5] = {8, 10, 12, 30, 20};
    char fields[5][31];  // 임시 저장 공간

    // 필드 값 복사 및 공백 채우기
    snprintf(fields[0], sizeof(fields[0]), "%-8s", s->sid);
    snprintf(fields[1], sizeof(fields[1]), "%-10s", s->name);
    snprintf(fields[2], sizeof(fields[2]), "%-12s", s->dept);
    snprintf(fields[3], sizeof(fields[3]), "%-30s", s->addr);
    snprintf(fields[4], sizeof(fields[4]), "%-20s", s->email);

    // recordbuf 구성
    int pos = 0;
    for (int i = 0; i < 5; i++) {
        strncpy(recordbuf + pos, fields[i], sizes[i]);
        pos += sizes[i];
        recordbuf[pos++] = '#';  // 구분자 추가
    }
    recordbuf[pos] = '\0';
}

// 함수 append: 학생 레코드 파일에 새로운 레코드 추가 (C library 사용)
int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email) {
    if (fp == NULL) return 0;

    int totalRecords = getRecordCount(fp);

    STUDENT s;
    strncpy(s.sid, sid, sizeof(s.sid) - 1);
    s.sid[sizeof(s.sid) - 1] = '\0';
    strncpy(s.name, name, sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';
    strncpy(s.dept, dept, sizeof(s.dept) - 1);
    s.dept[sizeof(s.dept) - 1] = '\0';
    strncpy(s.addr, addr, sizeof(s.addr) - 1);
    s.addr[sizeof(s.addr) - 1] = '\0';
    strncpy(s.email, email, sizeof(s.email) - 1);
    s.email[sizeof(s.email) - 1] = '\0';

    int success = writeRecord(fp, &s, totalRecords);
    if (!success) return 0;

    setRecordCount(fp, totalRecords + 1);
    return 1;
}

// 함수 search: 학생 레코드 파일에서 검색 키값을 만족하는 레코드를 sequential search (C library 사용)
void search(FILE *fp, enum FIELD f, char *keyval) {
    if (fp == NULL || keyval == NULL) return;

    int totalRecords = getRecordCount(fp);
    if (totalRecords == 0) {
        printf("#records = 0\n");
        return;
    }

    STUDENT *results[totalRecords];  // 결과를 저장할 배열
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
            case ADDR:
                if (strcmp(temp.addr, keyval) == 0) match = 1;
                break;
            case EMAIL:
                if (strcmp(temp.email, keyval) == 0) match = 1;
                break;
            default:
                break;
        }
        if (match) {
            results[foundCount] = malloc(sizeof(STUDENT));
            if (results[foundCount] == NULL) {
                perror("Memory allocation error");
                exit(1);
            }
            *(results[foundCount]) = temp;  // 구조체 복사
            foundCount++;
        }
    }

    print((const STUDENT **)results, foundCount);

    // 동적 할당된 메모리 해제
    for (int i = 0; i < foundCount; i++) {
        free(results[i]);
    }
}

// 함수 getFieldID: 레코드의 필드명을 enum FIELD 타입의 값으로 변환
enum FIELD getFieldID(char *fieldname) {
    if (strcmp(fieldname, "SID") == 0) return SID;
    else if (strcmp(fieldname, "NAME") == 0) return NAME;
    else if (strcmp(fieldname, "DEPT") == 0) return DEPT;
    else if (strcmp(fieldname, "ADDR") == 0) return ADDR;
    else if (strcmp(fieldname, "EMAIL") == 0) return EMAIL;
    else return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Append: %s -a record_file \"sid\" \"name\" \"dept\" \"addr\" \"email\"\n", argv[0]);
        fprintf(stderr, "  Search: %s -s record_file \"FIELD=keyvalue\"\n", argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *filename = argv[2];

    // 파일 열기 (r+b 모드: 읽기/쓰기, 바이너리)
    FILE *fp = fopen(filename, "r+b");
    if (fp == NULL) {
        // 파일이 없으면 새로 생성 (w+b 모드: 읽기/쓰기, 바이너리)
        fp = fopen(filename, "w+b");
        if (fp == NULL) {
            perror("File open error");
            return 1;
        }
        // 헤더 레코드 초기화: 레코드 수 0으로 설정
        int initialCount = 0;
        fwrite(&initialCount, sizeof(int), 1, fp);
        int reserved = 0;  // 예약 공간 0으로 초기화
        fwrite(&reserved, sizeof(int), 1, fp);
        fflush(fp);  // 버퍼 비우기
    }

    if (strcmp(mode, "-a") == 0) {
        if (argc != 8) {
            fprintf(stderr, "Append mode requires 5 fields\n");
            fclose(fp);
            return 1;
        }
        // append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email)
        if (!append(fp, argv[3], argv[4], argv[5], argv[6], argv[7])) {
            fprintf(stderr, "Append failed\n");
            fclose(fp);
            return 1;
        }
        // append 모드에서는 출력이 없음
    } else if (strcmp(mode, "-s") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Search mode requires 1 field condition\n");
            fclose(fp);
            return 1;
        }
        // search: $ a.out –s record_file_name “field_name=field_value”
        char *condition = argv[3];
        char *equalSign = strchr(condition, '=');  // "=" 위치 찾기
        if (equalSign == NULL) {
            fprintf(stderr, "Invalid search condition format\n");
            fclose(fp);
            return 1;
        }

        *equalSign = '\0';  // "="을 NULL 문자로 변경하여 fieldname 분리
        char *fieldname = condition;  // fieldname 포인터 설정
        char *keyval = equalSign + 1;  // keyval 포인터 설정

        enum FIELD field = getFieldID(fieldname);
        if (field == -1) {
            fprintf(stderr, "Invalid field name\n");
            fclose(fp);
            return 1;
        }
        // search() 호출
        search(fp, field, keyval);
    } else {
        fprintf(stderr, "Invalid mode\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);  // 파일 닫기
    return 0;
}

void print(const STUDENT *s[], int n) {
    printf("#records = %d\n", n);
    for (int i = 0; i < n; i++) {
        printf("%s#%s#%s#%s#%s#\n",
               s[i]->sid, s[i]->name, s[i]->dept, s[i]->addr, s[i]->email);
    }
}
