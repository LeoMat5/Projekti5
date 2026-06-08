#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Alustus
    FILE *fRead = NULL; // Jos ihmetyttää miksi nimeän esim fRead ohjelmassa, jossa käytetään "fread()" funktiota, fRead = File Read = luettava tiedosto
    int iAmountOfCharacters = 0; // RLE käänteisesti, tämä on merkkien määrä.
    char cSingleCharacter;       // RLE käänteisesti, tämä on itse merkki.
    int count = 1;               // Alkioiden määrä.
    
    if (argc > 1) { // Hyväksyttävä määrä syötteitä
        for (int i = 1; i < argc; i++) { // Iteroidaan argumentteja, i = 0 olisi ajettava tiedosto, joten hypätään sen yli.
            // Avataan luettavat tiedostot yksitellen.
            if ((fRead = fopen(argv[i], "r")) == NULL) { 
                printf("punzip: cannot open file\n");
                exit(1);
            }

        }
    
    } else { // Ei hyväksyttävä määrä syötteitä
        printf("pzip: file1 [file2 ...]\n");
        return(1);
    }
    return(0);
}