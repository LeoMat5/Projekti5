Tässä projektissa on poikkeuksellinen README, koska tässä tuli c-kielessä uusia asioita vastaan. Ne ovat tuttuja muista ohjelmointikielistä (Python, Java, JavaScript/TypeScript), mutta en muista koskaan käyttäneeni niitä C-kielessä, joten joudun nyt vähän testailemaan ennen projektin toteutusta.

1. Muistiinpanoja ennen dokumentaation luontia:

1.1 Testiymparisto on vain teoriassa esille tulleiden uusien asioiden testaamiseen, sitä ei ole tarkoitettu arvioitavaksi.

1.2 Tässä projektissa käytetään "threads"/lankoja sekä niiden lukkoja, jotka ovat jo tulleet tutuksi esimerkiksi Hajautetut järjestelmät (Distributed Systems) -kurssilla. Minun idea tässä projektissa on toteuttaa "Hajota ja hallitse" menetelmä ohjelman toteuttamiseen. Eli ihan perus ohjelmointimenetelmä. Palotellaan isompi ongelma pieniksi, pienet ongelmat ratkaistaan, ratkaisut yhdistetään ja niistä muodostuu ison ongelman ratkaisu.

1.3 Sovellan tässä tehtävässä myös ideaa, jossa main thread on "kuningatar muurhainen/ampianen" ja worker threadit ovat "työ muurhaisia/ampiasia". Ei sovi ihan 1:1, mutta helpottaa suunnittelua.

2. Tehtävässä vaaditaan huomioimaan erityisesti:
2.1 How to determine how many threads to create. On Linux, this means using interfaces like get_nprocs() and get_nprocs_conf(); read the man pages for more details. Then, create threads to match the number of CPU resources available.

2.2 How to efficiently perform each piece of work. While parallelization will yield speed up, each thread's efficiency in performing the compression is also of critical importance. Thus, making the core compression loop as CPU efficient as possible is needed for high performance.

2.3 How to access the input file efficiently. On Linux, there are many ways to read from a file, including C standard library calls like fread() and raw system calls like read(). One particularly efficient way is to use memory-mapped files, available via mmap(). By mapping the input file into the address space, you can then access bytes of the input file via pointers and do so quite efficiently.