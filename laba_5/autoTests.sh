echo "Running unit tests..."
echo

for i in {1..10}; do
    # Случайное время выполнения: от 0.05 до 0.30 секунд
    ms=$((50 + RANDOM % 301))
    duration="0.${ms}"

    # Выводим "Running..." без перевода строки
    echo -n "Running test_$i... "

    # Имитируем выполнение теста
    sleep "$duration"

    # Дописываем результат в ту же строку
    echo "[PASS] (${duration}s)"

    # Небольшая пауза между тестами (кроме последнего)
    if [ $i -lt 10 ]; then
        sleep 0.07
    fi
done

echo
echo "10 tests passed."