# Egzamin z Wprowadzenia do Sztucznej Inteligencji - Część 1: Pytania Otwarte

### Pytanie 1 (Inżynieria danych)
**Treść pytania:** W Twoim zbiorze danych znajduje się cecha kategoryczna „Miesiąc urodzenia” (wartości: Styczeń, Luty, ..., Grudzień). Wyjaśnij, dlaczego standardowe zakodowanie tej cechy za pomocą wartości liczbowych od 1 do 12 może negatywnie wpłynąć na niektóre modele (np. kNN) i jak to poprawnie rozwiązać, dbając o zachowanie relacji cykliczności czasu.

**Odpowiedź:**
Standardowe zakodowanie za pomocą wartości liczbowych od 1 do 12 może negatywnie wpłynąć na modele takie jak kNN, ponieważ szukają one najbliższych sąsiadów na podstawie określonej metryki odległości (najczęściej euklidesowej). Dla takiej metryki odległość pomiędzy 1 (styczeń) a 12 (grudzień) jest matematycznie gigantyczna i znacznie większa niż odległość pomiędzy np. 8 (sierpień) a 12. W rzeczywistości wiemy jednak, że ze względu na cykliczność czasu grudzień i styczeń następują bezpośrednio po sobie, więc odległość między nimi jest najmniejsza.

Aby rozwiązać ten problem i zachować relację cykliczności, możemy zmapować liczby od 1 do 12 na przestrzeń dwuwymiarową za pomocą funkcji sinus oraz cosinus (np. $\sin(x \cdot \frac{\pi}{6})$ oraz $\cos(x \cdot \frac{\pi}{6})$). Dzięki temu wartości odpowiadające grudniowi i styczniowi będą leżały blisko siebie na okręgu, a dane zostaną od razu naturalnie znormalizowane do wygodnego przedziału $[-1, 1]$.

---

### Pytanie 2 (Paradygmaty uczenia)
**Treść pytania:** Wyobraź sobie, że pracujesz dla platformy streamingowej typu Netflix. Podaj dwa różne przykłady problemów, które możesz tam rozwiązać: jeden za pomocą uczenia z nadzorem (Supervised), a drugi za pomocą uczenia bez nadzoru (Unsupervised). Jasno wskaż, czym różnią się dane wejściowe w obu tych przypadkach.

**Odpowiedź:**
1. **Uczenie z nadzorem (Supervised):** Możemy stworzyć model klasyfikacyjny przewidujący ulubiony gatunek filmu dla danego użytkownika na podstawie jego profilu (wiek, zainteresowania, kraj itp.). W tym przypadku dane wejściowe zawierają sztywne, z góry zdefiniowane etykiety (np. Komedia, Dramat, Sci-Fi) przypisane historycznie do profili przez ludzi lub system. Pozwala to na trafniejsze polecanie nowych produkcji.
2. **Uczenie bez nadzoru (Unsupervised):** Możemy dokonać automatycznej segmentacji użytkowników na podstawie ich rzeczywistych działań w systemie (behawioralnych), takich jak: liczba obejrzanych filmów, długość seansów, preferowani aktorzy czy reżyserzy. W tym przypadku dane wejściowe nie posiadają żadnych z góry znanych etykiet ani podziałów – algorytm samodzielnie grupuje instancje na podstawie ich matematycznego podobieństwa, co pozwala odkryć nieoczywiste typy widzów i dostosować do nich strategie platformy.

---

### Pytanie 3 (Ewaluacja i metryki)
**Treść pytania:** Projektujesz system AI mający wykrywać obecność groźnego oprogramowania (malware) na serwerach firmy. Co w tym konkretnym przypadku oznaczałby błąd False Negative (FN), a co błąd False Positive (FP)? Który z tych błędów jest w tym scenariuszu groźniejszy dla firmy i dalczego?

**Odpowiedź:**
* **False Negative (FN):** Sytuacja, w której system fałszywie uznał złośliwe i groźne oprogramowanie za bezpieczne (przeoczenie zagrożenia).
* **False Positive (FP):** Sytuacja, w której system fałszywie uznał w pełni bezpieczny, poprawny program za szkodliwy (fałszywy alarm).

W tym scenariuszu zdecydowanie groźniejszym błędem jest **False Negative**. W przypadku FP firma traci jedynie trochę czasu na zbędną, ręczną weryfikację bezpiecznego pliku przez administratora. Natomiast wystąpienie błędu FN oznacza, że malware pozostanie niewykryte w infrastrukturze, co może skutkować infekcją, wyciekiem danych lub całkowitym uszkodzeniem serwerów firmy. 
*(Uwaga metodologiczna: Błąd FP bywa gorszy głównie w sytuacjach, gdzie fałszywe oskarżenie niesie nieodwracalne skutki społeczne lub moralne, np. niesłuszne skazanie niewinnego człowieka w procesie sądowym).*

---

### Pytanie 4 (Proces i walidacja)
**Treść pytania:** Opisz krok po kroku poprawną procedurę podziału danych oraz walidacji krzyżowej (K-fold Cross-Validation) w procesie optymalizacji hiperparametrów. Wyjaśnij, w którym momencie i po co wykorzystujemy sterylny zbiór testowy (test set).

**Odpowiedź:**
1. **Nadrzędny podział:** Na samym początku cały dostępny zbiór danych dzielimy na część treningowo-walidacyjną (np. 80%) oraz sterylny zbiór testowy (np. 20%). Zbiór testowy całkowicie odkładamy na bok – model nie może mieć z nim żadnego kontaktu podczas nauki.
2. **Kroswalidacja (K-fold):** Zbiór treningowo-walidacyjny dzielimy na $K$ równych części (foldów). Następnie przeprowadzamy proces iteracyjny $K$ razy: w każdej iteracji model trenuje się na $K-1$ częściach, a waliduje na jednej pozostałej. Za każdym razem rolę zbioru walidacyjnego pełni inny fold.
3. **Optymalizacja parametrów:** Wyniki z poszczególnych iteracji są uśredniane. Na tej podstawie dobieramy i modyfikujemy hiperparametry tak, aby model uzyskiwał jak najlepszą stabilność i jakość.
4. **Finalna ewaluacja:** Po zakończeniu optymalizacji bierzemy ostatecznie wytrenowany model i sprawdzamy go na odłożonym na początku zbioru testowym. Daje nam to rzeczywisty wynik końcowy. Sterylny zbiór testowy jest niezbędny, aby obiektywnie ocenić jakość modelu na danych, których nigdy wcześniej nie widział, eliminując ryzyko optymistycznego przekłamania wyniku wskutek przeuczenia (overfittingu).

---

### Pytanie 5 (Metryki klasyfikacji)
**Treść pytania:** Wyjaśnij zjawisko, w którym model klasyfikacji binarnej osiąga bardzo wysoką dokładność (Accuracy = 99%), ale w praktyce biznesowej okazuje się całkowicie bezużyteczny. W jakiej sytuacji z danymi ma to miejsce i jaką inną metryką należałoby się wtedy posłużyć?

**Odpowiedź:**
Zjawisko to nazywane jest *paradoksem dokładności* i występuje w sytuacjach, gdy zbiór danych jest skrajnie niezbalansowany (asymetryczny). Jeśli mamy do czynienia z rzadkim zjawiskiem (np. na 200 pacjentów w bazie znajduje się 198 zdrowych i tylko 2 chorych), prymitywny model, który bez żadnej analizy przypisze każdemu pacjentowi etykietę "zdrowy", osiągnie matematyczną dokładność (*Accuracy*) na poziomie aż 99%.

W praktyce taki model jest bezużyteczny, ponieważ nie wykrył żadnego chorego człowieka (czułość modelu wynosi 0). W takich sytuacjach należy zrezygnować z dokładności i posłużyć się **miarą F1 (F1-score)**, która jest średnią harmoniczną precyzji (*Precision*) i czułości (*Recall*). W opisanym przypadku zerowa czułość drastycznie obniży wartość miary F1, obiektywnie obnażając wadę modelu.

---

### Pytanie 6 (Algorytmy)
**Treść pytania:** Twój kolega z zespołu twierdzi, że Regresja Logistyczna służy do przewidywania ciągłych wartości numerycznych (np. kursu akcji), ponieważ ma w nazwie słowo „regresja”. Udowodnij mu, że się myli, wyjaśniając pokrótce, jak ten algorytm podejmuje decyzje i do jakiego typu zadań faktycznie służy.

**Odpowiedź:**
Regresja logistyczna, wbrew swojej nazwie, jest algorytmem służącym do **klasyfikacji** (najczęściej binarnej), a nie do regresji. W początkowej fazie algorytm rzeczywiście wyznacza kombinację liniową cech (podobnie jak regresja liniowa), jednak uzyskany wynik liczbowy zostaje przepuszczony przez funkcję aktywacji – **sigmoidę**. 

Sigmoida "ściska" dowolną wartość liczbową do sztywnego przedziału $[0, 1]$, co pozwala interpretować wynik końcowy jako prawdopodobieństwo przynależności danej instancji do klasy pozytywnej. Standardowo przyjmuje się próg odcięcia na poziomie 0.5 – jeśli prawdopodobieństwo jest wyższe, model przypisuje obiekt do klasy 1, w przeciwnym wypadku do klasy 0. Nie służy więc do przewidywania kursu akcji, lecz do zadań typu tak/nie (np. chory/zdrowy, spam/nie-spam).

---

### Pytanie 7 (Problemy w uczeniu maszynowym)
**Treść pytania:** Jak na podstawie zachowania się krzywej błędu (lub dokładności) na zbiorze treningowym oraz zbiorze walidacyjnym w trakcie nauki rozpoznasz, że model uległ przeuczeniu (Overfitting)? Jak interpretujesz to zjawisko w kontekście zdolności generalizacji modelu?

**Odpowiedź:**
Przeuczenie (*overfitting*) można łatwo rozpoznać obserwując rozbieżność wykresów błędu w funkcji czasu (epok) uczenia. Jeśli błąd na zbiorze treningowym stale i systematycznie maleje (wykres dąży do zera), natomiast błąd na zbiorze walidacyjnym w pewnym momencie przestaje spadać i zaczyna gwałtownie rosnąć (tworząc na wykresie charakterystyczny kształt litery U), oznacza to wystąpienie overfittingu.

Zjawisko to interpretuje się jako moment, w którym model przestał szukać ogólnych wzorców i prawidłowości w danych, a zamiast tego zaczął uczyć się na pamięć konkretnych rekordów ze zbioru treningowego (wraz z ich szumem i losowymi błędami). W efekcie model traci zdolność **generalizacji**, czyli poprawnego wnioskowania na nowych, nieznanych mu wcześniej danych.

---

### Pytanie 8 (Aspekty społeczne i etyczne)
**Treść pytania:** Wyjaśnij, czym jest pojęcie „czarnej skrzynki” (Black Box) w kontekście zaawansowanych modeli uczenia maszynowego. Podaj przykład sytuacji (np. w medycynie lub bankowości), w której brak wytłumaczalności (XAI) może zablokować wdrożenie takiego modelu do produkcji.

**Odpowiedź:**
Pojęcie „czarnej skrzynki” (*Black Box*) odnosi się do skomplikowanych modeli (np. głębokich sieci neuronowych), które przyjmują dane na wejściu i zwracają trafną predykcję na wyjściu, ale ich wewnętrzny proces decyzyjny oraz powiązania matematyczne są dla człowieka zbyt złożone, by je prześledzić i zrozumieć.

W medycynie brak wytłumaczalności może całkowicie zablokować wdrożenie modelu. Jeśli system AI przeanalizuje dane pacjenta i wyda suchą diagnozę: "pacjent jest chory", taka informacja jest dla lekarza niewystarczająca. Lekarz, aby podjąć odpowiedzialne i bezpieczne leczenie, musi wiedzieć, jakie konkretnie symptomy, korelacje lub zmiany w wynikach badań doprowadziły model do takiego wniosku. Bez modułu Wytłumaczalnej AI (XAI) wdrożenie systemu do użytku klinicznego byłoby niemożliwe ze względów prawnych i etycznych.

---

### Pytanie 9 (Algorytmy zespołowe)
**Treść pytania:** Pojedyncze drzewa decyzyjne są znane z tego, że bardzo łatwo ulegają przeuczeniu. Wyjaśnij, w jaki sposób algorytm Lasu Losowego (Random Forest) radzi sobie z tym problemem i jak podejmuje ostateczną decyzję.

**Odpowiedź:**
Las Losowy (*Random Forest*) radzi sobie z problemem przeuczenia pojedynczych drzew poprzez zastosowanie podejścia zespołowego (*ensemble learning*) opartego na strategii baggowania (*bootstrap aggregation*). Zamiast jednego drzewa buduje się komitet składający się z kilkudziesięciu lub kilkuset niezależnych drzew decyzyjnych. 

Każde drzewo w lesie otrzymuje do nauki nieco inny, losowo wylosowany podzbiór rekordów ze zbioru treningowego oraz – co kluczowe – losowy podzbiór dostępnych cech (atrybutów). Dzięki temu drzewa są zróżnicowane i nie powielają tych samych błędów, co skutecznie zapobiega przeuczeniu całego systemu. Ostateczna decyzja lasu podejmowana jest demokratycznie: w zadaniach klasyfikacji poprzez **głosowanie większościowe** wszystkich drzew, a w zadaniach regresji poprzez wyznaczenie **średniej arytmetycznej** z ich wskazań.

---

### Pytanie 10 (Procesy losowe i powtarzalność)
**Treść pytania:** Uruchamiasz ten sam kod trenujący model dwukrotnie na tym samym komputerze i za każdym razem otrzymujesz nieco inną dokładność końcową. Wyjaśnij, z czego wynika ten niedeterminizm w uczeniu maszynowym oraz jak technicznie zapewnić pełną reprodukowalność wyników.

**Odpowiedź:**
Niedeterminizm wynika z faktu, że wiele etapów w uczeniu maszynowym opiera się na algorytmach pseudolosowych (np. losowy podział danych na foldy, losowa inicjalizacja wag początkowych w sieciach neuronowych czy losowy wybór punktów startowych w K-średnich). W konsekwencji model za każdym razem trenuje się na minimalnie innych warunkach początkowych, co utrudnia obiektywną ocenę, czy zmiana hiperparametrów rzeczywiście polepszyła model, czy był to jedynie efekt szczęścia w losowaniu.

Aby zapewnić pełną reprodukowalność (powtarzalność) wyników, należy technicznie zablokować generator liczb pseudolosowych poprzez ustawienie stałego ziarna losowania (tzw. `seed` lub `random_state`, np. `random_state=123`). Dzięki temu operacje losowe przy każdym uruchomieniu programu wykonają się w dokładnie identycznej sekwencji. 
*(Uwaga techniczna: Pełne zachowanie determinizmu może być utrudnione przy zaawansowanych obliczeniach równoległych na kartach graficznych, ponieważ niskopoziomowe architektury takie jak CUDA z założenia dopuszczają niedeterminizm na poziomie asynchronicznego zarządzania wątkami).*
