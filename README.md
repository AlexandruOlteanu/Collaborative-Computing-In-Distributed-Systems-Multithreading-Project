<Copyright Alexandru Olteanu, grupa 332CA, alexandruolteanu2001@gmail.com>

    In implementarea acestei teme am folosit MPI-uri pentru ca crea si
implementa procesele de la nivelul programului. Asa cum a fost precizat
in cerinta, am impartit procesele in doua categorii: manageri si workeri.
    Cerintele au fost rezolvate in modul urmator:
I. Afisarea topologiei de catre toate procesele.
    Pentru inceput am aflat topologia fiecarui cluster in parte prin 
parcurgerea fisierelor corespunzatoare de catre procesele manager. 
Dupa realizarea citirii am trimis datele intre procesele manager urmarind
configuratia de inel. Fiind 4 manageri, unele informatii au trebuit transportate
prin noduri intermediare, astfel ca m-am asigurat ca trecerea datelor se face 
cronologic astfel incat mereu managerul unui cluster are informatiile care i se 
cer la un moment dat. Dupa trimiterea datelor intre manageri mai ramane ca 
workerii sa stie topologia. Pentru a putea informa un worker in legatura cu 
topologia a trebuit sa o primeasca de la coordonatorul sau. O problema este 
ca workerii nu stiu ce coordonator au asa ca la operatia de MPI_Recv a topologiei
a trebuit sa deschidem o ascultare de la toate nodurile si astfel sa primeasca 
mesajul corect. Pentru a fi si informat de coordonatorul sau, ID-ul acestuia 
i-a fost trimis in interiorul mesajului. Odata ce toate procesele au aflat 
topologia au afisat-o. De asemenea, la fiecare trimitere de mesaje intre doua
procese, au fost afisate in log.

II. Pentru calculul vectorului am facut initializarea in cadrul procesului 0 
dupa care am trimis date fiecarui worker din cadrul acestuia si un interval
pe care fiecare din ei trebuie sa il rezolve. Intervalul a fost repartizat
aproximativ egal in functie de numarul total de procese worker. La randul lor, 
aceste procese au realizat inmultirea vectorului cu 5 in intervalul specificat 
dupa care au raspuns inapoi coordonatorului cu datele updatate. Dupa ce un 
coordonator a strans toate datele de la workeri le trimite mai departe 
pe ruta urmatoare -> 0 -> 3 -> 2 -> 1 -> 0. Astfel, cand datele ajung a doua oara
la procesul 0, vectorul este rezolvat complet si poate fi afisat.

III. In cadrul unei erori de comunicare intre procesele 0 si 1 am realizat 
intreaga comunicare dar cu o ruta modificata astfel incat datele sa ajunga
in continuare in mod corect. Spre exemplu, in aflarea topologiei, datele lui
0 trebuiau sa parcurga drumul 0 -> 3 -> 2 -> 1 pentru a ajunge la 1 in loc 
de ruta 0 -> 1 care era posibila initial. De asemenea, pentru calcularea rezultatelor
am urmat ruta 0 -> 3 -> 2 -> 1 -> 2 -> 3 -> 0 care are acelasi rezultat, tranzitiile
din urma avand ca rol doar transferul de date.

IV. In cadrul bonusului am considerat procesul 1 total separat de celelalte astfel ca
atat rutele s-au schimbat cat si topologiile afisate. Astfel, s-au afisat doua tipuri
de topologii, cea a lui 0, 2, 3, si cea a lui 1. Avand in vedere aceste modificari dese
ale rutelor am incercat sa realizez o abordare cat mai generica care sa poate fi modificata
in viitor in functie de noi reguli.

    Per total, o tema buna ce a abordat scopul materiei corespunzator, thx!
        