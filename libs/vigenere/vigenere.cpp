#include "../../include/cipher_interface.h"
#include <string>
#include <stdexcept>

using namespace std;

// универсальная реализация шифра виженера на уровне байтов (mod 256)
// не использует явный алфавит, как требует тз

string process_vigenere(const string& text, const string& key, bool encrypt_mode) {
    if (key.empty()) {
        // эта проверка важна, чтобы избежать деления на ноль в i % key.length()
        return "Ошибка: ключ не может быть пустым.";
    }

    string result = "";
    for (size_t i = 0; i < text.length(); ++i) {
        // получаем байты (коды символов)
        unsigned char text_char = text[i];
        unsigned char key_char = key[i % key.length()];

        unsigned char processed_char;
        if (encrypt_mode) {
            // шифрование: (текст + ключ) mod 256
            processed_char = (text_char + key_char) % 256;
        } else {
            // дешифрование: (текст - ключ + 256) mod 256
            // +256 нужно, чтобы избежать отрицательных результатов
            processed_char = (text_char - key_char + 256) % 256;
        }
        result += processed_char;
    }
    return result;
}


extern "C" {
    EXPORT_API const char* get_name() {
        return "Шифр Виженера (по ТЗ)";
    }

    EXPORT_API string encrypt(const string& text, const string& key) {
        return process_vigenere(text, key, true);
    }

    EXPORT_API string decrypt(const string& text, const string& key) {
        return process_vigenere(text, key, false);
    }

    EXPORT_API const char* get_key_hint() {
        return "Требуется ключевое слово (например, 'SECRET'). Работает с любыми символами.";
    }
}
