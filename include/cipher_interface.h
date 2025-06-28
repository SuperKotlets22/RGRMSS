#ifndef CIPHER_INTERFACE_H
#define CIPHER_INTERFACE_H

#include <string>

// макрос для корректного экспорта функций из dll/so
#if defined(_WIN32)
    #define EXPORT_API __declspec(dllexport)
#else
    #define EXPORT_API
#endif

// используем c-связывание, чтобы избежать искажения имен функций компилятором c++
// это критически важно для динамической загрузки
extern "C" {
    /**
     * @brief возвращает название алгоритма.
     * @return константная c-строка с именем.
     */
    EXPORT_API const char* get_name();

    /**
     * @brief шифрует переданный текст.
     * @param text исходный текст для шифрования.
     * @param key ключ шифрования.
     * @return зашифрованная строка.
     */
    EXPORT_API std::string encrypt(const std::string& text, const std::string& key);

    /**
     * @brief дешифрует переданный текст.
     * @param text зашифрованный текст для дешифрования.
     * @param key ключ шифрования.
     * @return исходная строка.
     */
    EXPORT_API std::string decrypt(const std::string& text, const std::string& key);

    /**
     * @brief предоставляет подсказку о формате ключа для генератора.
     * @return строка с подсказкой.
     */
    EXPORT_API const char* get_key_hint();
}

#endif // CIPHER_INTERFACE_H
