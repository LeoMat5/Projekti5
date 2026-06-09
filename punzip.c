#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////
//                                    HUOM!                                       //
//                                                                                //
//  Käytän tässä paljon samoja asioita kuin pzip:ssä, joten en kommentoi aina     //
//  pzipissä selitettyjä/kommentoituja asioita uudestaan.                         //
//  Edellisen ohjelman koodin uudelleenkäyttö sen takia, että jos asia on jo      //
//  ratkaistu yhdellä tavalla, sitä ei kannata käydä ratkaisemaan toisella.       //
//  Plus ohjelmoinnin ensimmäinen sääntö: Ohjelmoija on aina laiska.              //
////////////////////////////////////////////////////////////////////////////////////

// Structit/Tietueet //
// Tarvitaan structi, joka ylläpitää merkkej ja niiden määriä.
typedef struct {
    char *characters;
    int characterCounter;
} UnzippingResults;

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
    UnzippingResults *unzippingResults;
    pthread_mutex_t mutualExclusionLock;
} WorkingOrder;

// WorkInformation structin idea: määritellään kuka tekee, missä työ sijaitsee, työn koko ja järjestys. 
typedef struct {// https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf sivut 3-4. Idea siis määritellä työntekijöille työt näin. Eli yksi työntekijä säije voi saada useamman arvon.
    char *pFile;             
    WorkingOrder *order;
} WorkInformation;

// Työntekijän työt
void *tWorker(void *arg) { // my-unzipistä otettu paljon mallia
    WorkInformation *workInfo = arg; // https://pages.cs.wisc.edu/%7Eremzi/OSTEP/threads-api.pdf sivu 3
    PieceOfWork workPiece;
    UnzippingResults *unzippingResults = NULL;
    char *pFile = workInfo->pFile;
    size_t startPoint = 0;
    size_t endPoint = 0;
    size_t bytePiece = 0;
    int iWorkPieceToBeHandled = 0;
    int iWorkAvailable = 1;
    int iCharacterCount = 0;
    int iAmountOfWork = 0;
    char cSingleCharacter = '\0';
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
            unzippingResults = &workInfo->order->unzippingResults[iWorkPieceToBeHandled];
            
            
            // Työpalanen haettu, työstetään se. // Tämä on melkein sama kuin minun my-unzipissä, mutta vähän muokattuna.
            for (size_t i = startPoint; i < endPoint; i++) {
                bytePiece = i * (sizeof(int) + sizeof(char));
                
                iCharacterCount = 0;
                cSingleCharacter = '\0';
                
                // Kopioidaan muistia
                memcpy(&iCharacterCount, pFile + bytePiece, sizeof(int));
                cSingleCharacter = pFile[bytePiece + sizeof(int)];

                for (int j = 0; j < iCharacterCount; j++) {
                    unzippingResults->characters[unzippingResults->characterCounter] = cSingleCharacter;
                    unzippingResults->characterCounter++;
                }
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
    UnzippingResults *unzippingResults = NULL;
    int iFileDescriptor = -1;                                       // -1 = virhe, käytän sitä alustusarvona, jos se toimii testauksessa.
    struct stat sfileInformation;
    char *pMappedFile = NULL;                             
    int iSingleCharacterCount = 0;
    size_t workStart = 0;
    size_t workEnd = 0;
    size_t fileSize = 0;                                         
    size_t workSize = 0;
    size_t unzippedSize = 0;
    size_t bytePiece = 0;
    int iWorkPiecesPerWorker = 5;                                    // Tarvitsee tämmöisen kertoimen, jotta työtä jakaessa ei ole vain esim. yksi työpalanen per työntekijä, jolloin nopeammat eivät voi ottaa uusia.                                        
    int iAmountOfWork = 0;
    size_t pairSize = 0;                                            // määrä + merkki pari.
    size_t amountOfPairs = 0;                                       // Kuinka monta pareja on.


    if (argc > 1) { // Hyväksyttävä määrä syötteitä
        for (int i = 1; i < argc; i++) { // Iteroidaan argumentteja, i = 0 olisi ajettava tiedosto, joten hypätään sen yli.
            // Avataan luettavat tiedostot yksitellen.
            if ((iFileDescriptor = open(argv[i], O_RDONLY)) == -1) {  // O_RDONLY = Read only |  S_IRUSR = Lupa lukea | S_IWUSR = Lupa kirjoittaa
                printf("punzip: cannot open file\n");
                exit(1);
            }

            if (fstat(iFileDescriptor,&sfileInformation) == -1) {
                printf("Failure after open.\n");
            }

            // Tarvitaan tiedoston ja työn koko.
            fileSize = sfileInformation.st_size;                // Otetaan muuttujaan tiedoston koko.
            if (fileSize != 0) {                                // Tarkistetaan tyhjän tiedoston varalta.
                pMappedFile = mmap(NULL, sfileInformation.st_size, PROT_READ, MAP_PRIVATE, iFileDescriptor, 0);  // mmap(), kuten ohjeissa kehotettiin.
                
                pairSize = sizeof(int) + sizeof(char);            // Koska 4 tavuinen int + 1 tavuinen char = 5 tavuinen.
                amountOfPairs = fileSize / pairSize;
                iAmountOfWork = iAmountOfWorkersAvailable * iWorkPiecesPerWorker; // Jotta työjono olisi mahdollinen. Jos ei kerrottaisi, töitä olisi tarjolla 1 per prosessori, jolloin jono olisi turha.

                if ((size_t)iAmountOfWork > amountOfPairs) {
                    iAmountOfWork = (int)amountOfPairs;
                }

                workSize = (amountOfPairs + iAmountOfWork - 1) / iAmountOfWork; // Jakojäännöksen poisto.
                iAmountOfWork = (amountOfPairs + workSize - 1) / workSize;      
                PieceOfWork workPieces[iAmountOfWork];                          // Töiden paloitteluun.
                
                // Töiden paloittelu. Eli kuten Hajota ja hallitse menetelmässä: paloitellaan ongelma pienemmiksi osiksi, ratkaistaan osat ja myöhemmin kootaan.
                for (int i = 0; i < iAmountOfWork; i++) {
                    workStart = (size_t)i * workSize;
                    workEnd = workStart + workSize;

                    if (workEnd > amountOfPairs) {
                        workEnd = amountOfPairs;
                    }

                    workPieces[i].workStartPoint = workStart;
                    workPieces[i].workEndPoint = workEnd;
                }

                // Lukon alustus
                pthread_mutex_init(&order.mutualExclusionLock, NULL);

                // Muistin varaus
                if ((unzippingResults = malloc(sizeof(UnzippingResults) * iAmountOfWork)) == NULL) {
                        fprintf(stderr, "malloc failed 2\n");
                        exit(1);
                    }

                for (int i = 0; i < iAmountOfWork; i++) {
                    unzippedSize = 0;
                    workStart = workPieces[i].workStartPoint;
                    workEnd = workPieces[i].workEndPoint;
                    
                    for (size_t j = workStart; j < workEnd; j++) {
                        bytePiece = j * pairSize;   // Askelia siihen mennessä * askelten koko
                        iSingleCharacterCount = 0;
                        memcpy(&iSingleCharacterCount, pMappedFile + bytePiece, sizeof(int));

                        unzippedSize = unzippedSize + iSingleCharacterCount;
                    }

                    if ((unzippingResults[i].characters = malloc(sizeof(char) * unzippedSize)) == NULL) {
                        fprintf(stderr, "malloc failed 2\n");
                        exit(1);
                    }
                }
                
           // Tallennetaan structiin saadut tiedot.
                order.iAmountOfWork = iAmountOfWork;
                order.iNextPieceOfWork = 0;
                order.workPieces = workPieces;
                order.unzippingResults = unzippingResults;

                // Määritellään töitä ja työntekijöitä.
                for (int i = 0; i < iAmountOfWorkersAvailable; i++) {
                    workInfo[i].pFile = pMappedFile;
                    workInfo[i].order = &order;
                    
                    pthread_create(&workers[i], NULL, tWorker, &workInfo[i]);
                }

                // Odotetaan töiden valmistumista
                for (int i = 0; i < iAmountOfWorkersAvailable; i++) {
                    pthread_join(workers[i], NULL);
                }
            
                // Kootaan niiden tuloksista isomman ongelman vastaus Main Threadissa. Eli hajota ja hallitse menetelmän loppuosa.
                for (int i = 0; i < order.iAmountOfWork; i++) {
                    fwrite(order.unzippingResults[i].characters, sizeof(char), order.unzippingResults[i].characterCounter, stdout);

                }
                
                // Vapautetaan käytetty muisti
                for (int i = 0; i < order.iAmountOfWork; i++) {
                    free(unzippingResults[i].characters);
                }

                free(unzippingResults);
                unzippingResults = NULL;
                // currentUnzippingResults = NULL;
            }
            // Suljetaan aina avattu tiedosto
            munmap(pMappedFile, fileSize);
            close(iFileDescriptor);   
        }
    
    } else { // Ei hyväksyttävä määrä syötteitä
        printf("punzip: file1 [file2 ...]\n");
        return(1);
    }
    return(0);
}