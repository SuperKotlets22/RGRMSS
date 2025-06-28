#!/bin/bash

# --- конфигурация ---
# имя приложения, которое будет использоваться для создания каталогов и исполняемых файлов
APP_NAME="encryption-rgr"
# основной каталог для установки. /opt - стандартное место для сторонних приложений
INSTALL_DIR="/opt/${APP_NAME}"
# каталог для исполняемых файлов, доступных всем пользователям
BIN_DIR="/usr/local/bin"
# имя исполняемого файла приложения
MAIN_EXECUTABLE="main_app"
# имя исполняемого файла-пускача
LAUNCHER_NAME="encryption-rgr"

# --- цвета для вывода ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # нет цвета

# немедленно выходить, если команда завершается с ошибкой
set -e

# --- функция установки ---
install() {
    echo -e "${GREEN}Начинается установка ${APP_NAME}...${NC}"

    # 1. компиляция проекта
    echo -e "${YELLOW}Шаг 1/4: Компиляция проекта...${NC}"
    if [ ! -f "${MAIN_EXECUTABLE}" ]; then
        echo "Исполняемый файл '${MAIN_EXECUTABLE}' не найден. Запускаю сборку..."
        # компиляция библиотек
        g++ -shared -fPIC -o libs/atbash.so libs/atbash/atbash.cpp -I./
        g++ -shared -fPIC -o libs/transposition.so libs/transposition/transposition.cpp -I./
        g++ -shared -fPIC -o libs/vigenere.so libs/vigenere/vigenere.cpp -I./
        # компиляция основного приложения
        g++ -std=c++17 main.cpp -o ${MAIN_EXECUTABLE} -ldl
        echo "Сборка завершена."
    else
        echo "Исполняемый файл уже существует. Пропускаю сборку."
    fi

    # 2. создание каталогов и копирование файлов
    echo -e "${YELLOW}Шаг 2/4: Копирование файлов приложения...${NC}"
    echo "Создаю каталог ${INSTALL_DIR}"
    # `sudo` нужно здесь, т.к. мы пишем в системный каталог /opt
    sudo mkdir -p "${INSTALL_DIR}/libs"
    
    echo "Копирую исполняемый файл и библиотеки..."
    sudo cp "${MAIN_EXECUTABLE}" "${INSTALL_DIR}/"
    sudo cp -r libs/*.so "${INSTALL_DIR}/libs/"

    # 3. создание скрипта-запускатора
    # этот скрипт позволяет запускать приложение из любого места
    # он решает проблему с поиском библиотек в относительной папке ./libs
    echo -e "${YELLOW}Шаг 3/4: Создание команды для запуска...${NC}"
    echo "Создаю скрипт-запускатор в ${BIN_DIR}/${LAUNCHER_NAME}"
    # используем `tee` для записи в файл с правами sudo
    # EOF - это Here Document, позволяет передать многострочный текст как ввод
    tee <<EOF | sudo tee "${BIN_DIR}/${LAUNCHER_NAME}" > /dev/null
#!/bin/bash
# этот скрипт переходит в каталог приложения и запускает его
cd "${INSTALL_DIR}"
./${MAIN_EXECUTABLE} "\$@"
EOF

    # 4. предоставление прав на исполнение
    echo -e "${YELLOW}Шаг 4/4: Установка прав на исполнение...${NC}"
    sudo chmod +x "${BIN_DIR}/${LAUNCHER_NAME}"

    echo -e "\n${GREEN}Установка успешно завершена!${NC}"
    echo -e "Теперь вы можете запустить приложение из любого места, просто введя команду:"
    echo -e "  ${YELLOW}${LAUNCHER_NAME}${NC}"
}

# --- функция удаления ---
uninstall() {
    echo -e "${RED}Начинается удаление ${APP_NAME}...${NC}"
    
    # запрос подтверждения
    read -p "Вы уверены, что хотите полностью удалить приложение? [y/N] " confirm
    if [[ ! "$confirm" =~ ^[yY](es)?$ ]]; then
        echo "Удаление отменено."
        exit 0
    fi

    echo "Удаляю скрипт-запускатор..."
    sudo rm -f "${BIN_DIR}/${LAUNCHER_NAME}"

    echo "Удаляю каталог приложения..."
    sudo rm -rf "${INSTALL_DIR}"

    echo -e "\n${GREEN}${APP_NAME} успешно удален из системы.${NC}"
}

# --- главная логика скрипта ---
# проверка, что скрипт запущен с правами sudo
if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}Ошибка: этот скрипт необходимо запускать с правами суперпользователя.${NC}"
    echo "Пожалуйста, используйте: sudo ./setup.sh {install|uninstall}"
    exit 1
fi

# обработка аргументов командной строки
case "$1" in
    install)
        install
        ;;
    uninstall)
        uninstall
        ;;
    *)
        echo "Использование: sudo ./setup.sh {install|uninstall}"
        exit 1
        ;;
esac

exit 0
