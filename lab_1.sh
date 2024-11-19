#!/bin/bash

# Имена временных файлов
TEXT_FILE="/Users/aleksandr/Desktop/lab1/text.txt"
FIFO1="/Users/aleksandr/Desktop/lab1/fifo1"
FIFO2="/Users/aleksandr/Desktop/lab1/fifo2"

# Очистка перед запуском
cleanup() {
    echo "Прерывание: Удаление временных файлов..."
    rm -f $TEXT_FILE $FIFO1 $FIFO2
    exit 1
}

# Обработка сигнала SIGINT (Ctrl+C)
trap cleanup SIGINT

# Создание именованных каналов
mkfifo $FIFO1
mkfifo $FIFO2

# Процесс 1: Создание файла с текстом и запись текста в FIFO1
echo "first_123_lab    " > $TEXT_FILE &
cat $TEXT_FILE > $FIFO1 &

# Процесс 2: Чтение текста из FIFO1, добавление порядковых номеров, запись в FIFO2
(
  INPUT=$(cat $FIFO1)
  MODIFIED=""
  INDEX=1
  for (( i=0; i<${#INPUT}; i++ )); do
    CHAR="${INPUT:$i:1}"
    MODIFIED+="${CHAR}${INDEX}"
    ((INDEX++))
  done
  echo $MODIFIED > $FIFO2
) &

# Процесс 3: Чтение текста из FIFO2, корректная обработка символов и их индексов
(
  FORMATTED=""
  OUTPUT=$(cat $FIFO2)
  INDEX=1
  i=0
  while [ $i -lt ${#OUTPUT} ]; do
    # Берём текущий символ
    CHAR="${OUTPUT:$i:1}"
    ((i++))

    # Определяем разрядность текущего индекса
    INDEX_LENGTH=${#INDEX}

    # Читаем индекс (следующие INDEX_LENGTH символов)
    NUMBER="${OUTPUT:$i:$INDEX_LENGTH}"

    # Добавляем символ и индекс к результату
    FORMATTED+="$CHAR $NUMBER "

    # Сдвигаем позицию в строке
    i=$((i + INDEX_LENGTH))
    ((INDEX++))
  done
  echo "Результат: $FORMATTED"
) &

# Ожидание завершения процессов
wait

# Удаление временных файлов
cleanup

