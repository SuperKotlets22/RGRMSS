#include "../../include/cipher_interface.h"
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <locale>   // для конвертации
#include <codecvt>  // для конвертации

using namespace std;

// --- Внутренние функции, работающие с wstring ---

wstring encrypt_wide(const wstring& text, int num_cols) {
    wstring result = L"";
    result.reserve(text.length());
    for (int i = 0; i < num_cols; ++i) {
        for (size_t j = i; j < text.length(); j += num_cols) {
            result += text[j];
        }
    }
    return result;
}

wstring decrypt_wide(const wstring& text, int num_cols) {
    // вычисляем параметры сетки
    size_t text_len = text.length();
    if (text_len == 0) return L"";
    
    int num_rows = ceil(static_cast<double>(text_len) / num_cols);
    int num_full_cols = text_len % num_cols;
    if (num_full_cols == 0) {
        num_full_cols = num_cols;
    }

    wstring result(text_len, L' '); // инициализируем строку-результат
    int current_char_idx = 0;
    for (int i = 0; i < num_cols; ++i) {
        int col_len = (i < num_full_cols) ? num_rows : num_rows - 1;
        for (int j = 0; j < col_len; ++j) {
            // вычисляем позицию в исходной строке
            size_t pos = (size_t)j * num_cols + i;
            if (pos < text_len) {
                 result[pos] = text[current_char_idx++];
            }
        }
    }
    return result;
}


// --- Функция-обертка для конвертации и вызова нужной логики ---

string process_transposition_utf8(const string& text, const string& key, bool is_encrypt) {
    int num_cols;
    try {
        num_cols = stoi(key);
        if (num_cols <= 0) throw invalid_argument("");
    } catch (const exception&) {
        return "Ошибка: ключ должен быть положительным числом.";
    }

    try {
        // создаем конвертер для utf-8
        wstring_convert<codecvt_utf8<wchar_t>> converter;

        // 1. конвертируем входящую utf-8 строку в широкую строку
        wstring wide_text = converter.from_bytes(text);
        
        // 2. вызываем нужную функцию (шифрование или дешифрование)
        wstring wide_result;
        if (is_encrypt) {
            wide_result = encrypt_wide(wide_text, num_cols);
        } else {
            wide_result = decrypt_wide(wide_text, num_cols);
        }
        
        // 3. конвертируем результат обратно в utf-8 строку
        return converter.to_bytes(wide_result);
    } catch (const exception& e) {
        return "Ошибка кодировки в строке.";
    }
}


// --- Экспортируемые функции интерфейса ---

extern "C" {
    EXPORT_API const char* get_name() {
        return "Шифр простой перестановки";
    }

    EXPORT_API string encrypt(const string& text, const string& key) {
        return process_transposition_utf8(text, key, true);
    }

    EXPORT_API string decrypt(const string& text, const string& key) {
        return process_transposition_utf8(text, key, false);
    }

    EXPORT_API const char* get_key_hint() {
        return "Требуется числовой ключ (например, '5'), означающий количество столбцов.";
    }
}
