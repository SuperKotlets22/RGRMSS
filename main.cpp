#include <iostream>
#include <vector>
#include <string>
#include <filesystem> // для работы с файлами и директориями
#include <fstream>
#include <sstream>
#include <memory>

// заголовки для динамической загрузки библиотек
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#include "include/cipher_interface.h"

// для корректной работы с кириллицей в консоли windows
#ifdef _WIN32
void setup_console() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
}
#else
void setup_console() { /* на linux/macos обычно все работает из коробки с utf-8 */ }
#endif

using namespace std;

// определяем типы указателей на функции из наших библиотек
// это делает код более читаемым и безопасным
using F_get_name = const char* (*)();
using F_encrypt = string (*)(const string&, const string&);
using F_decrypt = string (*)(const string&, const string&);
using F_get_key_hint = const char* (*)();

// структура для хранения информации о загруженной библиотеке
struct CipherLibrary {
#ifdef _WIN32
    HINSTANCE handle;
#else
    void* handle;
#endif
    F_get_name get_name;
    F_encrypt encrypt;
    F_decrypt decrypt;
    F_get_key_hint get_key_hint;

    ~CipherLibrary() {
        if (handle) {
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
        }
    }
};

// вектор для хранения всех успешно загруженных библиотек
vector<unique_ptr<CipherLibrary>> loaded_ciphers;

void load_libraries() {
    // п. 2 ТЗ: приложение использует динамически подключаемые библиотеки
    string path = "./libs"; // предполагаем, что библиотеки лежат в папке libs
    cout << "Загрузка библиотек из '" << path << "'..." << endl;
    try {
        if (!filesystem::exists(path) || !filesystem::is_directory(path)) {
            cerr << "Предупреждение: директория '" << path << "' не найдена." << endl;
            return;
        }

        for (const auto& entry : filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
#ifdef _WIN32
                if (entry.path().extension() == ".dll") {
#else
                if (entry.path().extension() == ".so") {
#endif
                    auto lib = make_unique<CipherLibrary>();
#ifdef _WIN32
                    lib->handle = LoadLibrary(entry.path().string().c_str());
#else
                    lib->handle = dlopen(entry.path().string().c_str(), RTLD_LAZY);
#endif
                    if (!lib->handle) {
                        cerr << "Не удалось загрузить библиотеку: " << entry.path().string() << endl;
                        continue;
                    }

#ifdef _WIN32
                    lib->get_name = (F_get_name)GetProcAddress(lib->handle, "get_name");
                    lib->encrypt = (F_encrypt)GetProcAddress(lib->handle, "encrypt");
                    lib->decrypt = (F_decrypt)GetProcAddress(lib->handle, "decrypt");
                    lib->get_key_hint = (F_get_key_hint)GetProcAddress(lib->handle, "get_key_hint");
#else
                    lib->get_name = (F_get_name)dlsym(lib->handle, "get_name");
                    lib->encrypt = (F_encrypt)dlsym(lib->handle, "encrypt");
                    lib->decrypt = (F_decrypt)dlsym(lib->handle, "decrypt");
                    lib->get_key_hint = (F_get_key_hint)dlsym(lib->handle, "get_key_hint");
#endif
                    // проверка, что все функции найдены
                    if (lib->get_name && lib->encrypt && lib->decrypt && lib->get_key_hint) {
                        cout << "  - Загружен шифр: " << lib->get_name() << endl;
                        loaded_ciphers.push_back(move(lib));
                    } else {
                        cerr << "Ошибка: библиотека " << entry.path().string() << " не соответствует интерфейсу." << endl;
                    }
                }
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Ошибка при доступе к директории библиотек: " << e.what() << endl;
    }
    cout << "Загрузка завершена." << endl << endl;
}

CipherLibrary* choose_cipher() {
    if (loaded_ciphers.empty()) {
        cout << "Нет доступных шифров для работы." << endl;
        return nullptr;
    }
    cout << "Выберите алгоритм шифрования:" << endl;
    for (size_t i = 0; i < loaded_ciphers.size(); ++i) {
        cout << "  " << i + 1 << ". " << loaded_ciphers[i]->get_name() << endl;
    }
    cout << "> ";
    int choice;
    cin >> choice;
    // очистка буфера ввода
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

    if (choice > 0 && choice <= loaded_ciphers.size()) {
        return loaded_ciphers[choice - 1].get();
    } else {
        cout << "Неверный выбор." << endl;
        return nullptr;
    }
}

// п. 5.1.2. Процесс шифрования и дешифрования текста
void handle_text_operation() {
    CipherLibrary* cipher = choose_cipher();
    if (!cipher) return;

    cout << "Введите текст: ";
    string text;
    getline(cin, text);

    cout << "Введите ключ (" << cipher->get_key_hint() << "): ";
    string key;
    getline(cin, key);

    cout << "Выберите операцию (1 - Шифровать, 2 - Дешифровать): ";
    int mode;
    cin >> mode;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string result;
    if (mode == 1) {
        result = cipher->encrypt(text, key);
        cout << "Результат шифрования:" << endl;
    } else if (mode == 2) {
        result = cipher->decrypt(text, key);
        cout << "Результат дешифрования:" << endl;
    } else {
        cout << "Неверный выбор операции." << endl;
        return;
    }
    cout << result << endl;
}

// п. 5.1.3. Процесс шифрования и дешифрования файла
void handle_file_operation() {
    CipherLibrary* cipher = choose_cipher();
    if (!cipher) return;

    cout << "Введите путь к исходному файлу: ";
    string input_path;
    getline(cin, input_path);

    // п. 5.1.3.о: проверка существования файла
    if (!filesystem::exists(input_path)) {
        cout << "Ошибка: файл '" << input_path << "' не существует." << endl;
        return;
    }

    // чтение файла в бинарном режиме для универсальности
    ifstream input_file(input_path, ios::binary);
    if (!input_file) {
        cout << "Ошибка: не удалось открыть файл для чтения." << endl;
        return;
    }
    stringstream buffer;
    buffer << input_file.rdbuf();
    string content = buffer.str();
    input_file.close();
    
    cout << "Файл успешно прочитан (" << content.length() << " байт)." << endl;

    cout << "Введите путь к результирующему файлу: ";
    string output_path;
    getline(cin, output_path);

    cout << "Введите ключ (" << cipher->get_key_hint() << "): ";
    string key;
    getline(cin, key);

    cout << "Выберите операцию (1 - Шифровать, 2 - Дешифровать): ";
    int mode;
    cin >> mode;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string result_content;
    if (mode == 1) {
        result_content = cipher->encrypt(content, key);
        cout << "Файл зашифрован." << endl;
    } else if (mode == 2) {
        result_content = cipher->decrypt(content, key);
        cout << "Файл дешифрован." << endl;
    } else {
        cout << "Неверный выбор операции." << endl;
        return;
    }
    
    // п. 5.1.3.о: создание директорий при необходимости
    filesystem::path out_p(output_path);
    if (out_p.has_parent_path() && !filesystem::exists(out_p.parent_path())) {
        filesystem::create_directories(out_p.parent_path());
        cout << "Создана директория: " << out_p.parent_path().string() << endl;
    }

    ofstream output_file(output_path, ios::binary);
    if (!output_file) {
        cout << "Ошибка: не удалось создать/открыть файл для записи." << endl;
        return;
    }
    output_file << result_content;
    output_file.close();

    cout << "Результат сохранен в файл: " << output_path << endl;
}

// п. 5.1.4. Генератор ключей (в данном случае - вывод подсказок)
void handle_key_generator() {
     if (loaded_ciphers.empty()) {
        cout << "Сначала должны быть загружены шифры." << endl;
        return;
    }
    cout << "Подсказки по формату ключей для доступных алгоритмов:" << endl;
    for (const auto& lib : loaded_ciphers) {
        cout << "  - " << lib->get_name() << ": " << lib->get_key_hint() << endl;
    }
}


int main() {
    setup_console();
    load_libraries();

    int choice;
    do {
        // п. 5.1.1. Общие функции приложения
        cout << "\n===== Encryption Algorithm RGR =====" << endl;
        cout << "1. Шифрование/дешифрование текста" << endl;
        cout << "2. Шифрование/дешифрование файла" << endl;
        cout << "3. Генератор ключей (показать подсказки)" << endl;
        cout << "0. Выход" << endl;
        cout << "==================================" << endl;
        cout << "> ";
        cin >> choice;
        // обработка некорректного ввода (не числа)
        if (cin.fail()) {
            cin.clear(); // сброс флага ошибки
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // очистка буфера
            choice = -1; // установка невалидного значения
        } else {
             cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }


        // п. 5.4. обработка исключительных ситуаций
        try {
            switch (choice) {
                case 1:
                    handle_text_operation();
                    break;
                case 2:
                    handle_file_operation();
                    break;
                case 3:
                    handle_key_generator();
                    break;
                case 0:
                    cout << "Завершение работы." << endl;
                    break;
                default:
                    cout << "Неверный выбор. Пожалуйста, попробуйте снова." << endl;
            }
        } catch (const exception& e) {
            // общий обработчик на случай непредвиденных ошибок
            cerr << "Произошла критическая ошибка: " << e.what() << endl;
        }
    } while (choice != 0);
    
    // очистка ресурсов
    loaded_ciphers.clear();

    return 0;
}
