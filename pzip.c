#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////////
// HUOM! TÄMÄN KOODIN POHJA ON KOPIO OMASTA MY-ZIP:STÄ, JOTA ON VAIN LAAJENNETTU! //
////////////////////////////////////////////////////////////////////////////////////
//                                MUUTA HUOMIOITAVAA                              //
//                                                                                //
//  Threads opetettu jo Distributed Systems -kurssilla.                           //
//  Thread Lock käytetty myös siellä, tosin eri ohjelmointikielellä.              //
////////////////////////////////////////////////////////////////////////////////////

// Käytin myös seuraavia asioita //
// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf
// https://en.cppreference.com/c/io/fgetc
// Käytän tässä fgetc() esim. getline() tai fgets sijaan, koska täytyy iteroida merkki merkiltä. 

int main(int argc, char *argv[]) {
    // Alustus
    int iAmountOfCharacters = 0;    // RLE tarvitsi merkkien lukumäärän.
    char cSingleCharacter;          // RLE tarvitse sen yhden merkin lukumäärän perään.
    int iCurrentCharacter = 0;      // Tarvitaan tuohon fgetc != EOF. EOF on "End Of File".
    char iPreviousCharacter = -1;   // Kun mennään tiedostoista toiseen, niin täytyi säilyttää edellisen tiedoston merkki, jos se jatkuu seuraavassa tiedostossa.
    FILE *fRead = NULL;
    int count = 1;                  // Käytetään fwritessä. Alkioiden määrä.


    // printf("Amount of arguments %d\n", argc); // Testaukseen tarkoitettu printti.
    if (argc > 1) { // Hyväksyttävä määrä syötteitä
        for (int i = 1; i < argc; i++) { // Iteroidaan argumentteja, i = 0 olisi ajettava tiedosto, joten hypätään sen yli.
            // printf("Argument %i is: %s\n", i, argv[i]); // Testaukseen tarkoitettu printti.

            // Avataan luettavat tiedostot yksitellen.
            if ((fRead = fopen(argv[i], "r")) == NULL) { 
                printf("my-zip: cannot open file\n");
                // perror("fopen");
                exit(1);
            }
            // printf("Read file opened\n"); // Testaukseen tarkoitettu printti.

            // Käydään iteroitavan yksittäisen tiedoston merkistöt läpi ja lisätään niitä .z tiedostoon.
            while ((iCurrentCharacter = fgetc(fRead)) != EOF) {// While, koska ei tiedetä kuinka monta merkkiä pitäisi iteroida. Tässä on otettu mallia tuosta HUOM! mainitsemasta linkistä.
                // printf("%c\n", iCharacter); // Testaukseen tarkoitettu printti.
                if (iPreviousCharacter == -1) {             // Jos ei ole edellistä merkkiä, niin se tarkoittaa sitä, että ei ole iteroitu vielä mitään. Joten tallennetaan edelliseen nykyinen.
                    iPreviousCharacter = iCurrentCharacter; // Otetaan iCurrentCharacter apumuuttujaan talteen.
                    iAmountOfCharacters = 1;                // Eli nyt on ensimmäinen merkki. Ei käytetä ++, koska se on virhealttiimpi.       
                } else if (iPreviousCharacter == iCurrentCharacter) { // Kun nykyinen merkki on sama kuin edellinen, eli vaikka luettavan tiedoston tapauksessa.
                    iAmountOfCharacters++;                            // Nostetaan merkkien määrää yhdellä, eli merkkilaskuri.
                } else if (iPreviousCharacter != iCurrentCharacter) { // Merkki vaihtuu, joten kirjoitetaan nyt lasketut merkit ja lisätään se merkki.
                    // castataan/valetaan kokonaisluku merkiksi.
                    cSingleCharacter = (char)iPreviousCharacter;      
                    // printf("%d%c", iAmountOfCharacters, cSingleCharacter); // Testaukseen tarkoitettu printti.
                    
                    // Valetaan int (chariksi) ja kirjoitetaan se stdoutiin.
                    // sizeof(int) on se 4bytes vaatimus. Ensimmäinen fwrite kirjoittaa sen numeron, ja toinen kirjoittaa sen merkin numeron perään (sizeof(char)). Merkki on sen 1 tavua, joten 4 + 1 = 5 tavua.
                    fwrite(&iAmountOfCharacters, sizeof(int), count, stdout); // fwrite esim. putchar() sijaan, koska ohjeistus. Täytyi kyllä lukea manpages tämän kohdalla.
                    fwrite(&cSingleCharacter, sizeof(char), count, stdout); // Kirjoitetaan se yksi merkki lukumäärän perään.
                    
                    // Otetaan nykyinen merrki taas talteen seuraavalle kierrokselle ja alustetaan laskuri ykköseksi, koska se ei mene enää tuohon if haaraan.
                    iPreviousCharacter = iCurrentCharacter;
                    iAmountOfCharacters = 1;    
                } else {
                    printf("We shouldn't get here\n"); // Testaukseen tarkoitettu printti.
                }
            }

            fclose(fRead); // Suljetaan aina luettu tiedosto.
        }
        
        // Perinteinen viimeinen silmukan vaihe jää tekemättä, joten tehdään se sitten silmukan ulkopuolella.
        // Tarkistetaan vielä, ettei tyhjää tiedostoa kirjoiteta, eli jos edellistä symbolia ei ole löydetty, ja sen pitäisi olla lukiessa aina aluksi se nykyinen.
        // Testauksessa nimittäin ilmeni, että sen tyhjän tiedoston varalta pitää tarkistaa.
        if (iPreviousCharacter != -1) { 
            cSingleCharacter = (char)iPreviousCharacter;
            fwrite(&iAmountOfCharacters, sizeof(int), count, stdout);
            fwrite(&cSingleCharacter, sizeof(char), count, stdout);
            // printf("%d%c", iAmountOfCharacters, cSingleCharacter); // Testaukseen tarkoitettu printti.
        }

    } else { // Ei hyväksyttävä määrä syötteitä
        printf("my-zip: file1 [file2 ...]\n");
        return(1);
    }
    return(0);
}