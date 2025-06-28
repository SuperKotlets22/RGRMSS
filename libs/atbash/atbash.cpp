#include "../../include/cipher_interface.h"
#include <string>

using namespace std;

// общая функция для шифрования/дешифрования, т.к. алгоритм симметричен
// работает с кодами символов, а не со строкой-алфавитом, как требует тз
string process_atbash(const string& text) {
    string result = "";
    for (char c : text) {
        // обрабатываем английские строчные буквы, используя их числовые коды
        if (c >= 'a' && c <= 'z') {
            result += 'a' + ('z' - c);
        }
        // обрабатываем английские заглавные буквы
        else if (c >= 'A' && c <= 'Z') {
            result += 'A' + ('Z' - c);
        }
        // примечание: реализация для кириллицы потребовала бы знание конкретной кодировки (cp1251, utf-8)
        // и нарушила бы простоту работы с байтами. данная реализация является универсальной для ascii.
        // символы, не входящие в диапазоны, остаются без изменений
        else {
            result += c;
        }
    }
    return result;
}

extern "C" {
    EXPORT_API const char* get_name() {
        return "Шифр Атбаш (по ТЗ)";
    }

    EXPORT_API string encrypt(const string& text, const string& key) {
        // ключ игнорируется
        return process_atbash(text);
    }

    EXPORT_API string decrypt(const string& text, const string& key) {
        // дешифрование аналогично шифрованию
        return process_atbash(text);
    }

    EXPORT_API const char* get_key_hint() {
        return "Ключ не требуется.";
    }
}
