Simulare Pod Ingust - Sincronizare cu Monitoare



Acest proiect este o rezolvare pentru problema "Podului Ingust" in limbajul C, folosind thread-uri si mecanismul de Monitor pentru sincronizare. L-am scris pentru a exersa lucrul cu mutex-uri si variabile conditie (pthread).



Despre ce este vorba:

Avem un pod pe care masini vor sa treaca dinspre Nord spre Sud si invers. Problema este ca podul are restrictii:

1\. Nu incap mai mult de 3 masini in acelasi timp.

2\. Nu pot trece masini din sensuri opuse simultan (daca se merge spre Nord, Sudul asteapta).

3\. Pentru a nu bloca traficul, am pus o limita: daca au trecut 5 masini consecutiv intr-o directie si cineva asteapta pe partea cealaltă, sensul se schimba obligatoriu (evitarea infometarii).



Cum functioneaza codul:

\- Am creat o structura de Monitor care protejeaza accesul la variabilele podului (numarul de masini, directia curenta).

\- Fiecare masina este un thread separat.

\- Cand o masina vrea sa intre, verifica daca are loc si daca e randul ei. Daca nu, asteapta la o coada (variabila conditie).

\- Cand o masina iese de pe pod, notifica ceilalti thread-uri ca s-a eliberat un loc sau ca podul e gol si se poate schimba sensul.

