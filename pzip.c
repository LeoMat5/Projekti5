#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

////////////////////////////////////////////////////////////////////////////////////
//                                    HUOM!                                       //
//                                                                                //
//  Threads opetettu jo Distributed Systems -kurssilla.                           //
//  Thread Lock käytetty myös siellä, tosin eri ohjelmointikielellä.              //
//  Vaikka nämä ovat tuttuja, niin C-kieli on silti haastavammasta päästä.        //
//  Nimeämisessä en käytä jotain kuvaamatonta muutaman kirjaimen yhdistelmää tai  //
//  yhtä merkkiä, yritän nimetä muuttujan merkityksen mukaisesti.                 //
////////////////////////////////////////////////////////////////////////////////////

// Käytin myös seuraavia asioita //
// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf
// https://en.cppreference.com/c/io/fgetc
// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-locks.pdf
// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-locks-usage.pdf
// https://www.youtube.com/watch?v=m7E9piHcfr4 Tämä auttoi open() funktionin uudelleen määrittelyssä ja muutenkin muokkauksessa.
// https://stackoverflow.com/questions/37104895/dividing-work-up-between-threads-pthread Täällä sanotaan, ettei MUTEXia edes tarvitsisi käyttää, jos työnjako onnistuu hyvin.

// Structit/Tietueet //
// Tarvitaan structi, joka ylläpitää merkkiä ja sen määrää.
typedef struct {
    int iCharacterCount;
    char cSingleCharacter;
} CharacterInfo;

// Rakenne, jossa on merkkien tiedot ja niiden määrät, voidaan yhdistää myöhemmin.
typedef struct {
    CharacterInfo *info;
    int characterInfoCounter;
} CharacterInfos;

// PieceOfWork structin idea: yllä olevassa linkissä sanotaan, että työt kannattaa jakaa palasiksi, joten jaan ne palasiksi.
typedef struct { // https://stackoverflow.com/questions/37104895/dividing-work-up-between-threads-pthread
    size_t workStartPoint;
    size_t workEndPoint;
} PieceOfWork;

// WorkingOrder structin idea: ohjeissa vinkattiin, että mitä tehdä nopeiden threadien kanssa. Teen kasan töitä, joita prosessorit saavat sitten "oma-aloitteisesti" tehdä.
typedef struct { // https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-locks-usage.pdf
    int iAmountOfWork;
    int iNextPieceOfWork;
    PieceOfWork *workPieces;
    CharacterInfos *characterInfos;
    pthread_mutex_t mutualExclusionLock;
} WorkingOrder;

// WorkInformation structin idea: määritellään kuka tekee, missä työ sijaitsee, työn koko ja järjestys. 
typedef struct {// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf sivut 3-4. Idea siis määritellä työntekijöille työt näin. Eli yksi työntekijä säije voi saada useamman arvon.
    char *pFile;             
    WorkingOrder *order;
} WorkInformation;

// Työntekijän työt
void *tWorker(void *arg) {
    WorkInformation *workInfo = arg; // https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf sivu 3
    PieceOfWork workPiece;
    CharacterInfos *characterInfos = NULL;
    char *pFile = workInfo->pFile;
    size_t startPoint = 0;
    size_t endPoint = 0;
    int iWorkPieceToBeHandled = 0;
    int iWorkAvailable = 1;
    char cCurrentCharacter;
    char cPreviousCharacter = "\0";
    int iCharacterCount = 0;
    int iAmountOfWork = 0;
    iAmountOfWork = workInfo->order->iAmountOfWork;

    // Otetaan työ työntekijälle lukkoa käyttäen, ettei "race condition" tapahdu.
    while (iWorkAvailable == 1) {
        pthread_mutex_lock(&workInfo->order->mutualExclusionLock);  // Lukitaan.

        iWorkPieceToBeHandled = workInfo->order->iNextPieceOfWork;  // Otetaan nykyinen jonossa oleva työ työntekijälle.
        workInfo->order->iNextPieceOfWork++;                        // Päivitetään, että seuraava työstettävä työ on tästä työstä seuraava.

        pthread_mutex_unlock(&workInfo->order->mutualExclusionLock); // Avataan.

        if (iWorkPieceToBeHandled >= iAmountOfWork) {
            iWorkAvailable = 0;
        } else {
            workPiece = workInfo->order->workPieces[iWorkPieceToBeHandled];
            startPoint = workPiece.workStartPoint;
            endPoint = workPiece.workEndPoint;
            characterInfos = &workInfo->order->characterInfos[iWorkPieceToBeHandled];
            // Työpalanen haettu, työstetään se. // Tämä on melkein sama kuin minun my-zipissä, mutta vähän muokattuna, koska käytetään mmap() eikä fgetc().
            for (size_t i = startPoint; i < endPoint; i++) {
                char currentCharacter = pFile[i];
                
                if (iCharacterCount == 0) { // Aloitustilanne.
                    cPreviousCharacter = cCurrentCharacter;
                    iCharacterCount = 1;
                } else if (cPreviousCharacter == cCurrentCharacter) {   // Sama merkki
                    iCharacterCount++;                                  // Nostetaan merkkien määrää
                } else if (cPreviousCharacter != cCurrentCharacter) {   // Eri merkki
                    characterInfos->info[characterInfos->characterInfoCounter].cSingleCharacter = cPreviousCharacter;
                    characterInfos->info[characterInfos->characterInfoCounter].iCharacterCount = iCharacterCount;
                    characterInfos->characterInfoCounter++;

                    cPreviousCharacter = cCurrentCharacter;
                    iCharacterCount = 1;
                } else {
                    printf("We shouldn't get here\n"); // Testaukseen tarkoitettu printti.
                }
            }

            // Viimeinen taas silmukan ulkopuolella.
            if (iCharacterCount > 0) { // Ei käsitellä taaskaan tyhjiä tiedostoja.
                characterInfos->info[characterInfos->characterInfoCounter].cSingleCharacter = cPreviousCharacter;
                characterInfos->info[characterInfos->characterInfoCounter].iCharacterCount = iCharacterCount;
                characterInfos->characterInfoCounter++;
            }
        }
    }
    
    return(NULL);
}

int main(int argc, char *argv[]) { // Tämä on "Main Thread", eli "kuningatar" ampias/muurhais metaforassa.
    // Alustus
    int iAmountOfAvailableProcessors = get_nprocs();                // Nimen mukaisesti, käytettävissä olevat prosessorit.
    int iAmountOfWorkersAvailable = iAmountOfAvailableProcessors;   // Ehkä hieman turha ylimääräinen muuttuja, mutta helpottaa ymmärrystä.
    pthread_t workers[iAmountOfWorkersAvailable];                   // Lista työntekijöistäni.
    WorkInformation workInfo[iAmountOfWorkersAvailable];            // Työntekijöiden töiden tiedot. Tässä on olo kuin jollain projektipäälliköllä.
    WorkingOrder order;                                             // Työjärjestys jonoa varten.
    CharacterInfos *characterInfos = NULL;
    int iFileDescriptor = -1;                                       // -1 = virhe, käytän sitä alustusarvona, jos se toimii testauksessa.
    struct stat sfileInformation;
    char *pMappedFile = NULL;
    size_t fileSize = 0;                                         
    size_t workSize = 0;
    size_t workPiecesPerWorker = 5;                                 // Tarvitsee tämmöisen kertoimen, jotta työtä jakaessa ei ole vain esim. yksi työpalanen per työntekijä, jolloin nopeammat eivät voi ottaa uusia.                                        
    int iAmountOfWork = 0;
    size_t workPieceSize = 0;
    
    // printf("Amount of arguments %d\n", argc); // Testaukseen tarkoitettu printti.
    if (argc > 1) { // Hyväksyttävä määrä syötteitä
        for (int i = 1; i < argc; i++) { // Iteroidaan argumentteja, i = 0 olisi ajettava tiedosto, joten hypätään sen yli.
            
            // Avataan luettavat tiedostot yksitellen.
            // https://www.youtube.com/watch?v=m7E9piHcfr4
            if ((iFileDescriptor = open(argv[i], O_RDONLY)) == -1) {  // O_RDONLY = Read only |  S_IRUSR = Lupa lukea | S_IWUSR = Lupa kirjoittaa
                printf("pzip: cannot open file\n");
                exit(1);
            }
            
            if (fstat(iFileDescriptor,&sfileInformation) == -1) {
                printf("Failure after open.\n");
            }

            // Tarvitaan tiedoston ja työn koko.
            fileSize = sfileInformation.st_size;                // Otetaan muuttujaan tiedoston koko.
            if (fileSize != 0) {                                // Tarkistetaan tyhjän tiedoston varalta.
                workSize = fileSize/iAmountOfWorkersAvailable;  // Tiedoston koko jaettuna työntekijöiden määrällä. 

                // Kartoitetaan tiedot muistiin.
                pMappedFile = mmap(NULL, sfileInformation.st_size, PROT_READ, MAP_PRIVATE, iFileDescriptor, 0);  // mmap(, kuten ohjeissa kehotettiin.
                
                // Työmäärä
                iAmountOfWork = iAmountOfWorkersAvailable * workPiecesPerWorker; // Jotta työjono olisi mahdollinen. Jos ei kerrottaisi, töitä olisi tarjolla 1 per prosessori, jolloin jono olisi turha.

                if ((size_t)iAmountOfWork > fileSize) {
                    iAmountOfWork = (int)fileSize;
                }

                workSize = (fileSize + iAmountOfWork - 1) / iAmountOfWork; // Jakojäännöksen poisto.
                PieceOfWork workPieces[iAmountOfWork];                     // Töiden paloitteluun. Ei oikein voi määritellä aiemmin järkevästi.

                // Jaetaan nyt työpalaset niiden listaan, josta sitten jonossa niitä haetaan.
                for (int i = 0; i < iAmountOfWork; i++) {
                    workPieces[i].workStartPoint = i * workSize; // i on askeleen määrä ja workSize on askeleen koko.
                    workPieces[i].workEndPoint = (i + 1) * workSize; // (i + 1) on seuraavan palan alku.
                }

                // Viimeisen askeleen erillinen käsittely, koska se on parempi määritellä fileSizen loppuun.
                int lastWorkPiece = iAmountOfWork - 1;
                workPieces[lastWorkPiece].workStartPoint = lastWorkPiece * workSize; // i on askeleen määrä ja workSize on askeleen koko.
                workPieces[lastWorkPiece].workEndPoint = fileSize;

                // Lukon alustus:
                pthread_mutex_init(&order.mutualExclusionLock, NULL);

                // Tallennetaan structiin saadut tiedot.
                order.iAmountOfWork = iAmountOfWork;
                order.iNextPieceOfWork = 0;
                order.workPieces = workPieces;
                
                // Varataan tarvittava muisti
                if ((characterInfos = malloc(sizeof(CharacterInfos) * iAmountOfWork)) == NULL) {
                    fprintf(stderr, "malloc failed\n");
                    exit(1);
                }

                for (int i = 0; i < iAmountOfWork; i++) {
                    workPieceSize = workPieces[i].workEndPoint-workPieces[i].workStartPoint;

                    if ((characterInfos[i].info = malloc(sizeof(CharacterInfo) * workPieceSize)) == NULL) {
                    fprintf(stderr, "malloc failed\n");
                    exit(1);
                    }

                    characterInfos[i].characterInfoCounter = 0;
                }

                // Määritellään työntekijöitä.
                for (int i = 0; i < iAmountOfWorkersAvailable; i++) {
                    workInfo[i].pFile = pMappedFile;
                    workInfo[i].order = &order;
                    
                    pthread_create(&workers[i], NULL, tWorker, &workInfo[i]);
                }

                // Odotetaan töiden valmistumista ja kootaan niiden tuloksista isomman ongelman vastaus. Eli hajota ja hallitse menetelmä.
                for (int i = 0; i < iAmountOfWorkersAvailable; i++) {
                    pthread_join(workers[i], NULL);
                }

                // Vapautetaan käytetty muisti
                for (int i = 0; i < iAmountOfWork; i++) {
                    free(characterInfos[i].info);
                }

                free(characterInfos);
            }
                // Suljetaan aina avattu tiedosto
                close(iFileDescriptor);   
            }

    } else { // Ei hyväksyttävä määrä syötteitä
        printf("pzip: file1 [file2 ...]\n");
        return(1);
    }
    return(0);
}