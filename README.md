Pewnie — poniżej masz rozbudowaną notatkę z **TCP i UDP w C na Linuxie**, dokładnie pod Twoje laby/SOP-y.

---

# 1. Podstawy socketów

Socket to taki „uchwyt”/deskryptor pliku do komunikacji sieciowej.

Tworzysz go funkcją:

```c
int fd = socket(domain, type, protocol);
```

Najczęściej:

```c
socket(AF_INET, SOCK_STREAM, 0); // TCP IPv4
socket(AF_INET, SOCK_DGRAM, 0);  // UDP IPv4
```

Czyli:

```text
AF_INET      = IPv4
SOCK_STREAM  = TCP
SOCK_DGRAM   = UDP
```

Socket jest zwykłym deskryptorem pliku, więc można na nim robić:

```c
read(fd, ...)
write(fd, ...)
close(fd)
```

ale zależnie od TCP/UDP używa się też:

```c
recvfrom(...)
sendto(...)
connect(...)
accept(...)
```

---

# 2. Adres sieciowy `sockaddr_in`

Dla IPv4 używasz struktury:

```c
struct sockaddr_in addr;
```

Typowe wypełnienie:

```c
memset(&addr, 0, sizeof(addr));

addr.sin_family = AF_INET;
addr.sin_port = htons(port);
addr.sin_addr.s_addr = htonl(INADDR_ANY);
```

Znaczenie:

```text
sin_family       = rodzina adresów, np. AF_INET
sin_port         = port
sin_addr.s_addr  = adres IP
```

`INADDR_ANY` oznacza:

```text
nasłuchuj na wszystkich interfejsach sieciowych
```

czyli np.:

```text
127.0.0.1
192.168.x.x
adres z Ethernetu
adres z Wi-Fi
```

---

# 3. Kolejność bajtów: `htons`, `ntohs`, `htonl`, `ntohl`

W sieci używa się tzw. **network byte order**, czyli big endian.

Dlatego liczby wielobajtowe trzeba konwertować.

## Przed wysłaniem / wpisaniem do adresu

```c
htons(x) // host to network short, 16 bitów
htonl(x) // host to network long, 32 bity
```

Przykład:

```c
addr.sin_port = htons(2137);
addr.sin_addr.s_addr = htonl(INADDR_ANY);
```

## Po odebraniu z sieci

```c
ntohs(x) // network to host short
ntohl(x) // network to host long
```

Przykład:

```c
uint16_t net_port;
uint16_t port = ntohs(net_port);
```

Dla pojedynczego `char`/`uint8_t` **nie używasz konwersji**, bo to jest jeden bajt.

```c
char c;
read(fd, &c, 1); // bez htons/ntohs
```

Dla tekstu też nie używasz `htons`.

---

# 4. TCP — idea

TCP jest połączeniowy.

To znaczy, że zanim klient i serwer zaczną wymieniać dane, musi powstać połączenie.

Schemat:

```text
SERWER TCP:
socket()
bind()
listen()
accept()
read()/write()
close()

KLIENT TCP:
socket()
connect()
read()/write()
close()
```

---

# 5. TCP server — co robią funkcje?

## `socket()`

Tworzy socket.

```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
```

To jest jeszcze zwykły socket TCP, ale nie ma przypisanego portu.

---

## `bind()`

Przypina socket do lokalnego adresu i portu.

```c
bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
```

Czyli mówisz systemowi:

```text
ten serwer ma używać portu 2137
```

Przykład:

```c
addr.sin_family = AF_INET;
addr.sin_port = htons(2137);
addr.sin_addr.s_addr = htonl(INADDR_ANY);

bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
```

---

## `listen()`

Zmienia socket w socket nasłuchujący.

```c
listen(server_fd, backlog);
```

Od teraz socket czeka na połączenia TCP.

`backlog` to **rozmiar kolejki połączeń oczekujących na `accept()`**.

To nie jest dokładnie liczba klientów, których obsługujesz naraz.

Przykład:

```c
listen(server_fd, 10);
```

Znaczy mniej więcej:

```text
kernel może trzymać około 10 połączeń czekających na accept()
```

Schemat:

```text
client1 connect() ┐
client2 connect() ├──> kolejka backlog ──accept()──> client_fd
client3 connect() ┘
```

---

## `accept()`

Przyjmuje konkretnego klienta.

```c
int client_fd = accept(server_fd, NULL, NULL);
```

Ważne rozróżnienie:

```text
server_fd = socket serwera, który nasłuchuje
client_fd = socket konkretnego klienta
```

Po `accept()` komunikujesz się z klientem przez `client_fd`.

```c
read(client_fd, ...)
write(client_fd, ...)
```

A `server_fd` dalej służy do przyjmowania kolejnych klientów.

---

# 6. Minimalny TCP server — jeden klient naraz

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int make_tcp_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        die("socket");

    return fd;
}

int bind_tcp_socket(uint16_t port, int backlog)
{
    int fd = make_tcp_socket();

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        die("setsockopt");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(fd, backlog) < 0)
        die("listen");

    return fd;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint16_t port = (uint16_t)atoi(argv[1]);

    int server_fd = bind_tcp_socket(port, 10);

    printf("TCP server listening on port %d\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
            die("accept");

        printf("New client connected, fd=%d\n", client_fd);

        char buffer[BUFFER_SIZE];

        while (1) {
            ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);

            if (n < 0)
                die("read");

            if (n == 0) {
                printf("Client disconnected\n");
                break;
            }

            buffer[n] = '\0';

            printf("Client says: %s", buffer);

            if (write(client_fd, buffer, n) < 0)
                die("write");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
```

Kompilacja:

```bash
gcc tcp_server.c -o tcp_server
```

Uruchomienie:

```bash
./tcp_server 2137
```

Test przez netcat:

```bash
nc localhost 2137
```

---

# 7. TCP client

Klient robi:

```text
socket()
connect()
write()
read()
close()
```

## `connect()`

Łączy socket klienta z serwerem.

```c
connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
```

Po udanym `connect()` można robić:

```c
write(fd, ...)
read(fd, ...)
```

Ten sam deskryptor służy do wysyłania i odbierania.

---

# 8. Minimalny TCP client

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int make_tcp_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        die("socket");

    return fd;
}

struct sockaddr_in make_address(const char *ip, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
        die("inet_pton");

    return addr;
}

int connect_tcp_socket(const char *ip, uint16_t port)
{
    int fd = make_tcp_socket();

    struct sockaddr_in addr = make_address(ip, port);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("connect");

    return fd;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s ip port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);

    int fd = connect_tcp_socket(ip, port);

    char buffer[BUFFER_SIZE];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);

        if (write(fd, buffer, len) < 0)
            die("write");

        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

        if (n < 0)
            die("read");

        if (n == 0) {
            printf("Server disconnected\n");
            break;
        }

        buffer[n] = '\0';

        printf("Server replied: %s", buffer);
    }

    close(fd);
    return 0;
}
```

Kompilacja:

```bash
gcc tcp_client.c -o tcp_client
```

Uruchomienie:

```bash
./tcp_client 127.0.0.1 2137
```

---

# 9. TCP `read()` — ważne zachowania

```c
ssize_t n = read(fd, buffer, size);
```

Możliwe wyniki:

```text
n > 0   → odebrano n bajtów
n == 0  → druga strona zamknęła połączenie
n < 0   → błąd
```

Przykład:

```c
ssize_t n = read(client_fd, buffer, sizeof(buffer));

if (n > 0) {
    // odebrano dane
}
else if (n == 0) {
    // klient się rozłączył
}
else {
    // błąd
}
```

---

# 10. TCP to strumień bajtów

Bardzo ważne:

TCP **nie zachowuje granic wiadomości**.

Jeśli klient zrobi:

```c
write(fd, "hello", 5);
write(fd, "world", 5);
```

to serwer może dostać:

```text
helloworld
```

albo:

```text
hel
lowor
ld
```

albo jeszcze inaczej.

TCP gwarantuje kolejność bajtów, ale nie mówi:

```text
tu kończy się jedna wiadomość
```

Dlatego jeśli chcesz mieć wiadomości, musisz sam ustalić protokół, np.:

## Opcja 1 — wiadomości zakończone `\n`

Klient wysyła:

```text
hello\n
vote 1\n
```

Serwer czyta aż do znaku `\n`.

## Opcja 2 — stały rozmiar wiadomości

Każda wiadomość ma np. 32 bajty.

## Opcja 3 — długość na początku

Najpierw wysyłasz długość:

```text
[uint32_t length][payload]
```

np.:

```text
[00000005][hello]
```

To jest najbardziej profesjonalne.

---

# 11. `write()` też nie musi wysłać wszystkiego naraz

```c
write(fd, buffer, len);
```

może wysłać mniej bajtów niż `len`.

Dlatego robi się funkcję typu `bulk_write`.

```c
ssize_t bulk_write(int fd, const void *buf, size_t count)
{
    size_t written_total = 0;
    const char *ptr = buf;

    while (written_total < count) {
        ssize_t n = write(fd, ptr + written_total, count - written_total);

        if (n < 0) {
            if (errno == EINTR)
                continue;

            return -1;
        }

        if (n == 0)
            break;

        written_total += n;
    }

    return written_total;
}
```

Użycie:

```c
const char *msg = "hello\n";
bulk_write(fd, msg, strlen(msg));
```

---

# 12. `SIGPIPE`

Jeśli piszesz do TCP socketu, którego druga strona już zamknęła, proces może dostać `SIGPIPE`.

Domyślna akcja `SIGPIPE` to:

```text
zabicie procesu
```

Dlatego w serwerach często robi się:

```c
signal(SIGPIPE, SIG_IGN);
```

Wtedy `write()` zamiast ubić proces, zwróci:

```text
-1
errno = EPIPE
```

I możesz obsłużyć to normalnie:

```c
ssize_t n = write(fd, msg, len);

if (n < 0) {
    if (errno == EPIPE) {
        printf("Client disconnected\n");
        close(fd);
    }
}
```

---

# 13. TCP z wieloma klientami — problem blokowania

Jeśli masz wielu klientów i robisz:

```c
read(client1_fd, ...);
```

to przy blokującym sockecie program może się zatrzymać na `client1`, mimo że `client2` coś wysłał.

Dlatego używa się:

```text
select()
poll()
epoll()
wątki
procesy
```

Na Linuxie bardzo popularny jest `epoll`.

---

# 14. `epoll` — idea

`epoll` pozwala jednemu wątkowi obsługiwać wielu klientów.

Schemat:

```text
1. Tworzysz epoll_fd
2. Dodajesz server_fd do epolla
3. epoll_wait() czeka na zdarzenia
4. Jeśli zdarzenie na server_fd → accept()
5. Jeśli zdarzenie na client_fd → read()
```

Dla socketu serwera:

```text
EPOLLIN = ktoś chce się połączyć, można zrobić accept()
```

Dla socketu klienta:

```text
EPOLLIN = klient wysłał dane albo się rozłączył
```

---

# 15. Szkielet TCP servera z epoll

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#define MAX_EVENTS 16
#define BUFFER_SIZE 1024

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int make_tcp_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        die("socket");

    return fd;
}

int bind_tcp_socket(uint16_t port, int backlog)
{
    int fd = make_tcp_socket();

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        die("setsockopt");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(fd, backlog) < 0)
        die("listen");

    return fd;
}

void add_fd_to_epoll(int epoll_fd, int fd)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
        die("epoll_ctl ADD");
}

void remove_and_close_fd(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void handle_client(int epoll_fd, int client_fd)
{
    char buffer[BUFFER_SIZE];

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);

    if (n < 0) {
        perror("read");
        remove_and_close_fd(epoll_fd, client_fd);
        return;
    }

    if (n == 0) {
        printf("Client fd=%d disconnected\n", client_fd);
        remove_and_close_fd(epoll_fd, client_fd);
        return;
    }

    buffer[n] = '\0';

    printf("Client fd=%d says: %s", client_fd, buffer);

    if (write(client_fd, buffer, n) < 0) {
        perror("write");
        remove_and_close_fd(epoll_fd, client_fd);
    }
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint16_t port = (uint16_t)atoi(argv[1]);

    int server_fd = bind_tcp_socket(port, 10);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        die("epoll_create1");

    add_fd_to_epoll(epoll_fd, server_fd);

    printf("TCP epoll server listening on port %d\n", port);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (n < 0) {
            if (errno == EINTR)
                continue;

            die("epoll_wait");
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);

                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                printf("New client fd=%d\n", client_fd);

                add_fd_to_epoll(epoll_fd, client_fd);
            }
            else {
                handle_client(epoll_fd, fd);
            }
        }
    }

    close(epoll_fd);
    close(server_fd);

    return 0;
}
```

Kompilacja:

```bash
gcc tcp_epoll_server.c -o tcp_epoll_server
```

Uruchomienie:

```bash
./tcp_epoll_server 2137
```

Kilku klientów:

```bash
nc localhost 2137
```

w kilku terminalach.

---

# 16. UDP — idea

UDP jest bezpołączeniowy.

Nie ma:

```text
listen()
accept()
prawdziwego connect jak w TCP
gwarancji dostarczenia
gwarancji kolejności
strumienia bajtów
```

UDP wysyła osobne paczki zwane datagramami.

Schemat:

```text
SERWER UDP:
socket()
bind()
recvfrom()
sendto()

KLIENT UDP:
socket()
sendto()
recvfrom()
```

---

# 17. UDP vs TCP — najważniejsza różnica

## TCP

```text
połączenie
strumień bajtów
read/write
accept/connect
gwarantuje kolejność
gwarantuje dostarczenie albo błąd
```

## UDP

```text
brak połączenia
datagramy
sendto/recvfrom
brak accept/listen
brak gwarancji dostarczenia
brak gwarancji kolejności
możliwe duplikaty
```

---

# 18. UDP server

UDP server musi znać swój port.

Dlatego robi:

```c
socket(AF_INET, SOCK_DGRAM, 0);
bind(fd, ...);
```

Potem odbiera dane:

```c
recvfrom(fd, buffer, size, 0, (struct sockaddr *)&client_addr, &client_len);
```

`recvfrom()` daje Ci dwie rzeczy:

```text
1. dane
2. adres klienta, który wysłał datagram
```

Dzięki temu możesz odpowiedzieć:

```c
sendto(fd, response, len, 0, (struct sockaddr *)&client_addr, client_len);
```

---

# 19. Minimalny UDP server

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int bind_udp_socket(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        die("socket");

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        die("setsockopt");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    return fd;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint16_t port = (uint16_t)atoi(argv[1]);

    int fd = bind_udp_socket(port);

    printf("UDP server listening on port %d\n", port);

    char buffer[BUFFER_SIZE];

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ssize_t n = recvfrom(
            fd,
            buffer,
            sizeof(buffer) - 1,
            0,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (n < 0)
            die("recvfrom");

        buffer[n] = '\0';

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        printf("From %s:%d: %s",
               client_ip,
               ntohs(client_addr.sin_port),
               buffer);

        const char *response = "UDP server received your message\n";

        if (sendto(
                fd,
                response,
                strlen(response),
                0,
                (struct sockaddr *)&client_addr,
                client_len
            ) < 0)
            die("sendto");
    }

    close(fd);
    return 0;
}
```

Kompilacja:

```bash
gcc udp_server.c -o udp_server
```

Uruchomienie:

```bash
./udp_server 2137
```

Test przez netcat UDP:

```bash
nc -u localhost 2137
```

---

# 20. UDP client klasyczny — `sendto()`

Klient UDP zwykle nie musi robić `bind()`.

System sam wybierze mu lokalny port.

Klient robi:

```text
socket()
sendto(server_addr)
recvfrom()
close()
```

---

# 21. Minimalny UDP client z `sendto`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

struct sockaddr_in make_address(const char *ip, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
        die("inet_pton");

    return addr;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s ip port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        die("socket");

    struct sockaddr_in server_addr = make_address(ip, port);

    char buffer[BUFFER_SIZE];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (sendto(
                fd,
                buffer,
                strlen(buffer),
                0,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)
            ) < 0)
            die("sendto");

        ssize_t n = recvfrom(
            fd,
            buffer,
            sizeof(buffer) - 1,
            0,
            NULL,
            NULL
        );

        if (n < 0)
            die("recvfrom");

        buffer[n] = '\0';

        printf("Server replied: %s", buffer);
    }

    close(fd);
    return 0;
}
```

Kompilacja:

```bash
gcc udp_client.c -o udp_client
```

Uruchomienie:

```bash
./udp_client 127.0.0.1 2137
```

---

# 22. UDP i `bind()` po stronie klienta

Klient UDP może, ale nie musi, robić `bind()`.

## Bez `bind()`

System wybiera lokalny port automatycznie.

```text
client: 127.0.0.1: losowy_port
server: 127.0.0.1:2137
```

Np.:

```text
client: 127.0.0.1:51823
server: 127.0.0.1:2137
```

## Z `bind()`

Ty wymuszasz lokalny port klienta.

```c
bind(fd, ..., port 4321);
```

Wtedy serwer zobaczy, że wiadomość przyszła z:

```text
twoje_ip:4321
```

Przykład:

```c
int fd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in local_addr;
memset(&local_addr, 0, sizeof(local_addr));

local_addr.sin_family = AF_INET;
local_addr.sin_port = htons(4321);
local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
```

Czyli:

```text
bind() w UDP = ustaw mój lokalny port
```

---

# 23. UDP `connect()`

W UDP też można użyć `connect()`, ale to nie robi prawdziwego połączenia jak w TCP.

UDP `connect()` oznacza:

```text
ustaw domyślny adres zdalny dla tego socketu
```

Po UDP `connect()` możesz zamiast:

```c
sendto(fd, msg, len, 0, (struct sockaddr *)&addr, sizeof(addr));
recvfrom(fd, buffer, size, 0, NULL, NULL);
```

robić:

```c
write(fd, msg, len);
read(fd, buffer, size);
```

albo:

```c
send(fd, msg, len, 0);
recv(fd, buffer, size, 0);
```

---

# 24. UDP client z `connect()`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

struct sockaddr_in make_address(const char *ip, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
        die("inet_pton");

    return addr;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s ip port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        die("socket");

    struct sockaddr_in server_addr = make_address(ip, port);

    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die("connect");

    char buffer[BUFFER_SIZE];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (write(fd, buffer, strlen(buffer)) < 0)
            die("write");

        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

        if (n < 0)
            die("read");

        buffer[n] = '\0';

        printf("Server replied: %s", buffer);
    }

    close(fd);
    return 0;
}
```

Ważne:

```text
UDP connect nie daje gwarancji dostarczenia.
UDP dalej jest UDP.
Nie ma accept().
Nie ma listen().
```

---

# 25. UDP datagramy a TCP strumień

To jest bardzo ważne.

## TCP

TCP to strumień.

Jeśli wyślesz:

```text
abc
def
```

serwer może odebrać:

```text
abcdef
```

albo:

```text
ab
cdef
```

Granice wiadomości nie są zachowane.

## UDP

UDP zachowuje granice datagramów.

Jeśli zrobisz:

```c
sendto(fd, "abc", 3, ...);
sendto(fd, "def", 3, ...);
```

to odbiorca dostanie osobne datagramy:

```text
abc
def
```

Ale UDP może zgubić datagram.

---

# 26. UDP i za mały bufor

W TCP, jeśli masz bufor 8 bajtów, a przyszło 20 bajtów, to:

```text
read() odbierze 8
reszta zostanie na później
```

W UDP, jeśli datagram ma 20 bajtów, a `recvfrom()` ma bufor 8 bajtów, to zwykle:

```text
odbierzesz 8 bajtów,
reszta datagramu zostanie ucięta i utracona
```

To jest bardzo ważna różnica.

TCP:

```text
dane zostają w strumieniu
```

UDP:

```text
jeden recvfrom = jeden datagram
jeśli bufor za mały, reszta datagramu przepada
```

---

# 27. Typowe funkcje pomocnicze

## TCP bind

```c
int bind_tcp_socket(uint16_t port, int backlog_size)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) < 0)
        die("setsockopt");

    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(socketfd, backlog_size) < 0)
        die("listen");

    return socketfd;
}
```

---

## UDP bind

```c
int bind_udp_socket(uint16_t port)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;

    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd < 0)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) < 0)
        die("setsockopt");

    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    return socketfd;
}
```

---

## Uniwersalne bind dla TCP/UDP

To jest funkcja podobna do tej, którą miałeś:

```c
int bind_inet_socket(uint16_t port, int type, int backlog)
{
    struct sockaddr_in addr;
    int socketfd, t = 1;

    socketfd = socket(AF_INET, type, 0);
    if (socketfd < 0)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) < 0)
        die("setsockopt");

    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("bind");

    if (type == SOCK_STREAM) {
        if (listen(socketfd, backlog) < 0)
            die("listen");
    }

    return socketfd;
}
```

Użycie dla TCP:

```c
int tcp_fd = bind_inet_socket(2137, SOCK_STREAM, 10);
```

Użycie dla UDP:

```c
int udp_fd = bind_inet_socket(2137, SOCK_DGRAM, 0);
```

---

# 28. `setsockopt(SO_REUSEADDR)`

Często dodajesz:

```c
int t = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t));
```

Dzięki temu możesz szybciej ponownie uruchomić serwer na tym samym porcie.

Bez tego po zamknięciu serwera możesz dostać:

```text
bind: Address already in use
```

bo port może być jeszcze chwilę w stanie `TIME_WAIT`.

---

# 29. Blokujące i nieblokujące sockety

Domyślnie sockety są blokujące.

## Blokujący `accept()`

```c
accept(server_fd, NULL, NULL);
```

czeka aż pojawi się klient.

## Blokujący `read()`

```c
read(client_fd, buffer, size);
```

czeka aż:

```text
przyjdą dane
albo klient się rozłączy
albo wystąpi błąd
```

## Nieblokujący socket

Można ustawić:

```c
fcntl(fd, F_SETFL, O_NONBLOCK);
```

Wtedy jeśli nie ma danych:

```c
read(fd, buffer, size);
```

zwróci:

```text
-1
errno = EAGAIN albo EWOULDBLOCK
```

To znaczy:

```text
nie ma teraz danych, spróbuj później
```

---

# 30. `EINTR`

Wywołanie systemowe może zostać przerwane sygnałem.

Wtedy np.:

```c
read(...)
```

może zwrócić:

```text
-1
errno = EINTR
```

Często robi się:

```c
ssize_t n;

do {
    n = read(fd, buffer, size);
} while (n < 0 && errno == EINTR);
```

Albo używa się makra:

```c
TEMP_FAILURE_RETRY(...)
```

np.:

```c
int client_fd = TEMP_FAILURE_RETRY(accept(server_fd, NULL, NULL));
```

---

# 31. Najważniejsze porównanie TCP vs UDP

```text
TCP:
- połączeniowy
- connect/accept
- read/write
- gwarantuje kolejność bajtów
- gwarantuje dostarczenie albo błąd
- strumień bajtów
- brak granic wiadomości
- dobry do: SSH, HTTP, plików, komunikacji niezawodnej

UDP:
- bezpołączeniowy
- brak accept/listen
- sendto/recvfrom
- datagramy
- zachowuje granice datagramów
- może gubić pakiety
- może zmienić kolejność
- może mieć duplikaty
- dobry do: DNS, gry, streaming, własne protokoły z ACK
```

---

# 32. Miniściąga

## TCP server

```c
fd = socket(AF_INET, SOCK_STREAM, 0);
bind(fd, ...);
listen(fd, backlog);
client_fd = accept(fd, ...);
read(client_fd, ...);
write(client_fd, ...);
close(client_fd);
close(fd);
```

## TCP client

```c
fd = socket(AF_INET, SOCK_STREAM, 0);
connect(fd, server_addr, ...);
write(fd, ...);
read(fd, ...);
close(fd);
```

## UDP server

```c
fd = socket(AF_INET, SOCK_DGRAM, 0);
bind(fd, ...);
recvfrom(fd, buffer, ..., &client_addr, &client_len);
sendto(fd, response, ..., &client_addr, client_len);
close(fd);
```

## UDP client bez connect

```c
fd = socket(AF_INET, SOCK_DGRAM, 0);
sendto(fd, msg, ..., &server_addr, sizeof(server_addr));
recvfrom(fd, buffer, ..., NULL, NULL);
close(fd);
```

## UDP client z connect

```c
fd = socket(AF_INET, SOCK_DGRAM, 0);
connect(fd, &server_addr, sizeof(server_addr));
write(fd, msg, len);
read(fd, buffer, size);
close(fd);
```

---

# 33. Najważniejsze rzeczy do zapamiętania na kolokwium/lab

```text
bind()    = ustaw lokalny adres/port
connect() = połącz z adresem zdalnym / ustaw zdalny adres
listen()  = tylko TCP server, zacznij nasłuchiwać
accept()  = tylko TCP server, przyjmij konkretnego klienta
read()    = odbierz dane z połączonego socketu
write()   = wyślij dane po połączonym sockecie
recvfrom() = UDP, odbierz dane i adres nadawcy
sendto()   = UDP, wyślij dane pod konkretny adres
```

Dla TCP:

```text
server_fd służy do accept()
client_fd służy do komunikacji
```

Dla UDP:

```text
zwykle masz jeden socket,
a klientów rozpoznajesz po sockaddr_in z recvfrom()
```

Dla `read()` na TCP:

```text
> 0  = dane
== 0 = klient się rozłączył
< 0  = błąd
```

Dla `backlog`:

```text
backlog to kolejka połączeń czekających na accept(),
a nie liczba klientów obsługiwanych naraz.
```

Dla `SIGPIPE`:

```text
pisanie do zamkniętego TCP socketu może ubić proces,
dlatego często robi się signal(SIGPIPE, SIG_IGN).
```
