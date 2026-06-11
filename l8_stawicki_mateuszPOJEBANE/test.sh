
#!/bin/sh
set -eu

PORT="${1:-9000}"
HOST="${2:-127.0.0.1}"
NC_BIN="${NC_BIN:-nc}"

send_hex() {
    label="$1"
    hex="$2"

    printf '\n== %s ==\n' "$label"

    printf '%s' "$hex" |
        xxd -r -p |
        "$NC_BIN" -u -w1 "$HOST" "$PORT"
}

pad_hex() {
    python3 - "$1" "$2" <<'PY'
import sys

text = sys.argv[1].encode("ascii")
size = int(sys.argv[2])

if len(text) > size:
    raise SystemExit(f"field too long: {text!r}")

print((text + b"\0" * (size - len(text))).hex())
PY
}

u32be_hex() {
    python3 - "$1" <<'PY'
import struct
import sys

value = int(sys.argv[1], 0)

if not 0 <= value <= 0xffffffff:
    raise SystemExit(f"value out of range: {value}")

print(struct.pack("!I", value).hex())
PY
}

build_message_hex() {
    login="$1"
    cmd="$2"
    shift 2

    hex="$(pad_hex "$login" 16)$(pad_hex "$cmd" 8)"

    while [ "$#" -gt 0 ]; do
        hex="${hex}$(u32be_hex "$1")"
        shift
    done

    printf '%s' "$hex"
}

# Wysyła polecenie z konkretnego portu źródłowego i odbiera
# odpowiedzi na porcie source_port + 1.
#
# Odpowiedź LIST jest interpretowana jako ciąg par:
# [count:uint32_t][seed:uint32_t]
send_list_and_receive() {
    label="$1"
    login="$2"
    source_port="$3"

    hex="$(build_message_hex "$login" LIST)"

    printf '\n== %s ==\n' "$label"

    python3 - "$HOST" "$PORT" "$source_port" "$hex" <<'PY'
import socket
import struct
import sys

host = sys.argv[1]
server_port = int(sys.argv[2])
source_port = int(sys.argv[3])
payload = bytes.fromhex(sys.argv[4])

response_port = source_port + 1

receiver = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    # Najpierw uruchamiamy odbiornik, żeby nie zgubić szybkiej odpowiedzi.
    receiver.bind(("0.0.0.0", response_port))
    receiver.settimeout(1.0)

    # Polecenie LIST zostanie wysłane z jawnie ustawionego portu.
    sender.bind(("0.0.0.0", source_port))
    sender.sendto(payload, (host, server_port))

    datagram_number = 0
    jobs_number = 0

    while True:
        try:
            data, address = receiver.recvfrom(65535)
        except socket.timeout:
            break

        datagram_number += 1

        print(
            f"Datagram {datagram_number}: "
            f"{len(data)} bajtów, od {address[0]}:{address[1]}"
        )

        if len(data) == 0:
            print("  <pusty datagram>")
            continue

        if len(data) % 8 != 0:
            print(
                "  BŁĄD: rozmiar odpowiedzi nie jest "
                "wielokrotnością 8 bajtów"
            )
            print(f"  RAW: {data.hex()}")
            continue

        values = struct.unpack(f"!{len(data) // 4}I", data)

        for i in range(0, len(values), 2):
            count = values[i]
            seed = values[i + 1]
            jobs_number += 1

            print(
                f"  zadanie {jobs_number}: "
                f"count={count}, seed={seed}"
            )

    if datagram_number == 0:
        print(
            "Brak odpowiedzi. Użytkownik może nie mieć zadań "
            "albo serwer niczego nie wysłał."
        )
    else:
        print(
            f"Łącznie: {jobs_number} zadań "
            f"w {datagram_number} datagramach"
        )

finally:
    sender.close()
    receiver.close()
PY
}


VALID_LOGIN="krasowskip"
SECOND_LOGIN="hermant"
EMPTY_LOGIN="turs"
INVALID_LOGIN="not-in-logins"

printf '\n===== PODSTAWOWA WALIDACJA =====\n'

send_hex \
    "valid RUN" \
    "$(build_message_hex "$VALID_LOGIN" RUN)"
sleep 0.2

send_hex \
    "invalid RUN with parameters" \
    "$(build_message_hex "$VALID_LOGIN" RUN 123 456)"
sleep 0.2

send_hex \
    "invalid LIST with parameters" \
    "$(build_message_hex "$VALID_LOGIN" LIST 123 456)"
sleep 0.2

send_hex \
    "invalid COMPUTE without parameters" \
    "$(build_message_hex "$VALID_LOGIN" COMPUTE)"
sleep 0.2

send_hex \
    "invalid COMPUTE with one uint32 parameter" \
    "$(build_message_hex "$VALID_LOGIN" COMPUTE 10)"
sleep 0.2


printf '\n===== DODAWANIE ZADAŃ =====\n'

# Dwa poprawne zadania.
send_hex \
    "valid COMPUTE with two jobs" \
    "$(build_message_hex "$VALID_LOGIN" COMPUTE \
        5000 123 \
        1200 456)"
sleep 0.2

# Dokładnie 64 bajty:
# 16 B login + 8 B command + 10 * 4 B parametrów = 64 B.
# To jest maksymalna poprawna wiadomość i zawiera 5 zadań.
send_hex \
    "valid maximum-size COMPUTE with five jobs" \
    "$(build_message_hex "$VALID_LOGIN" COMPUTE \
        1 101 \
        2 102 \
        3 103 \
        4 104 \
        5 105)"
sleep 0.2

# Pierwsze i trzecie zadanie są poprawne.
# Drugie powinno zostać pominięte, bo count > 10 000 000.
send_hex \
    "mixed COMPUTE: valid, invalid, valid" \
    "$(build_message_hex "$VALID_LOGIN" COMPUTE \
        10000000 777 \
        10000001 999 \
        42 4242)"
sleep 0.2

# Zadania drugiego użytkownika — nie mogą pojawić się
# na liście krasowskip.
send_hex \
    "valid COMPUTE for second user" \
    "$(build_message_hex "$SECOND_LOGIN" COMPUTE \
        700 11 \
        800 22)"
sleep 0.2


printf '\n===== TESTY LIST =====\n'

# krasowskip ma łącznie 9 poprawnych zadań:
# 2 + 5 + 2.
#
# Jeden datagram mieści maksymalnie 8 zadań:
# 8 zadań * 8 bajtów = 64 bajty.
#
# Oczekujemy więc:
# - pierwszy datagram: 8 zadań / 64 bajty,
# - drugi datagram: 1 zadanie / 8 bajtów.
send_list_and_receive \
    "LIST krasowskip — expected 9 jobs in 2 datagrams" \
    "$VALID_LOGIN" \
    15000
sleep 0.2

# Powinny przyjść tylko dwa zadania użytkownika hermant.
send_list_and_receive \
    "LIST hermant — expected only his 2 jobs" \
    "$SECOND_LOGIN" \
    15010
sleep 0.2

# LIST nie powinien usuwać zadań.
# Drugie wywołanie powinno zwrócić ponownie te same 9 zadań.
send_list_and_receive \
    "second LIST krasowskip — queue should be unchanged" \
    "$VALID_LOGIN" \
    15020
sleep 0.2

# Ten użytkownik nie ma zadań.
# Przy obecnej implementacji serwer prawdopodobnie nic nie wyśle.
send_list_and_receive \
    "LIST user without jobs" \
    "$EMPTY_LOGIN" \
    15030
sleep 0.2


printf '\n===== DODATKOWE BŁĘDNE WIADOMOŚCI =====\n'

send_hex \
    "invalid unknown user" \
    "$(build_message_hex "$INVALID_LOGIN" LIST)"
sleep 0.2

send_hex \
    "invalid unknown command" \
    "$(build_message_hex "$VALID_LOGIN" STATUS)"
sleep 0.2

send_hex \
    "invalid too short datagram" \
    "0011223344556677"
sleep 0.2

# 11 parametrów uint32_t:
# 24 B nagłówka + 44 B parametrów = 68 B.
# Datagram przekracza MSG_MAX=64.
send_hex \
    "invalid datagram larger than MSG_MAX" \
    "$(build_message_hex "$VALID_LOGIN" COMPUTE \
        1 1 \
        2 2 \
        3 3 \
        4 4 \
        5 5 \
        6)"
sleep 0.2


printf '\n===== ZAKOŃCZENIE =====\n'

send_hex \
    "valid EXIT" \
    "$(build_message_hex "$VALID_LOGIN" EXIT)"
