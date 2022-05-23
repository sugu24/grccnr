#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DictItem DictItem;

struct DictItem {
    DictItem *next;
    char *key;
    int len;
    char *val_str;
    int val_num;
};

// keyの長さを返す
int key_len(char *key) {
    int len = 0;
    while (*key) {
        len++;
        key++;
    }
    return len;
}

// keyに対してハッシュ値を計算して返す
int hash_number(char *key, int table_len) {
    int hash = 0;
    while (*key) {
        hash = (hash * 29) + *key;
        hash %= table_len;
        key++;
    }
    return hash;
}

// ヒープ領域に連想配列分の領域を確保してそれをセットする
int create_dict(DictItem **dict, int table_len) {
    *dict = calloc(table_len, sizeof(DictItem));
}

// keyのval_numをvalで更新する 成功:1 失敗:0 を返す
int set_val_num(DictItem **dict, int table_len, char *key_s, int val) {
    int key = hash_number(key_s, table_len);
    int len = key_len(key_s);
    for (DictItem *item = (*dict + key); item; item = item->next) {
        if (item->len == len && !strncmp(item->key, key_s, len)) {
            item->val_num = val;
            return 1;
        }
    }
    return 0;
}

// keyがあるか返す 成功:1 失敗:0 を返す
int is_item(DictItem **dict, int table_len, char *key_s) {
    int key = hash_number(key_s, table_len);
    int len = key_len(key_s);
    for (DictItem *item = (*dict + key); item; item = item->next) {
        if (item->len == len && !strncmp(item->key, key_s, len)) {
            return 1;
        }
    }
    return 0;
}

// keyのval_numを返す
int get_val_num(DictItem **dict, int table_len, char *key_s) {
    int key = hash_number(key_s, table_len);
    int len = key_len(key_s);
    for (DictItem *item = (*dict + key); item; item = item->next) {
        if (item->len == len && !strncmp(item->key, key_s, len)) {
            return item->val_num;
        }
    }
    return 0;
}

// itemを追加
int add_item(DictItem **dict, int table_len, char *key_s) {
    int key = hash_number(key_s, table_len);
    printf("%d\n", key);
    int len = key_len(key_s);
    DictItem *item = (*dict + key);

    while (item->next) item = item->next;
    
    item->key = key_s;
    item->len = len;
    item->next = calloc(1, sizeof(DictItem));
    return 1;
}

DictItem *dict;

int main() {
    create_dict(&dict, 1024);
    add_item(&dict, 1024, "val");
    set_val_num(&dict, 1024, "val", 4);
    if (is_item(&dict, 1024, "val")) {
        printf("there is val\n");
        int val = get_val_num(&dict, 1024, "val");
        printf("var = %d\n", val);
    } 
    else 
        printf("there is not val\n");
}