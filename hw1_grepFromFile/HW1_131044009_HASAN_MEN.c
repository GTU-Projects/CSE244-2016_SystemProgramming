#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*DEFAULT LOG DOSYA ISMI */
#define LOG_FILE "gff.log"

/*TYPE DEFINE ENUMERATIONS */
typedef enum {
    false = 0, true = 1
} bool;

/*
 * Bu fonksiyon kendisine gelen string icinde istenilen noktadan itibaren kac
 * defa aranan kelime oldugunu bulur. Olmamasi durumunda -1 return eder.
 * Parameters : line -> arama yapilacak string, const char *
 * startPoint : hangi indisten arama islemine baslanacak
 * word : aranmasi istenilen kelime
 *
 */
int findOccurencesInLine(const char *line, int lineNumber, const char *word);


/* Bu fonksiyon kendisine gelen dosya ismine gore dosyayı acarak icinde
 * icinde istenilen kelimeyi arar ve normal modda acilmasi durumunda log
 * log dosyasine kelimenin bulundugu indisleri yazar.
 * Parameters : fileName -> arama yapilacak dosya
 * word : aranilacak kelimenin
 * logOrNormal : log file modunda 'l' acilinca sadece logun logunu olusturmaz
 * sadece log dosyasinda kac defa yazildigini return edecektir.
 * 'n' normal modda acilinca gecen kelimeleri extra olarak log dosyasina
 * kaydeder.
 */
int searchInFile(const char *fileName, const char *word, char logOrNormal);


/* Bu fonksiyon hangi dosya icin log kaydi yapilacagini ve onceden acilmis
 * yazilmis loglar icin yeni oldugunu belirtecek sekilde dosyaya ekleme yapar
 * Olusturdugu dosyanın file * return eder.
 * Parameters : searchedFileName -> hangi dosya icin log konrolu yapilip
 * log dosyasi acilacak
 * Return : Acilan log dosyasinin pointerini return edecek.
 */
FILE * createLogFile(const char* searchedFileName);

/* FONKSIYONLARI TEST ETMEK ICIN MAIN */
int main(int argc, char *argv[]) {

    /* USAGE GOSTERIMI*/
    if (argc < 3) {
        fprintf(stderr, "Usage: %s filename word\n", argv[0]);
        return -1;
    }

    /* Istenilen dosya icinde normal modda kelime aramasi yapilacaktir.*/
    printf("Total occurence of %s : %d times", argv[2],
            searchInFile(argv[1], argv[2], 'n'));

    return 0;
}

FILE * createLogFile(const char* searchedFileName) {

    FILE * fpLogFile;
    /* ESKI LOGLARIN KONTROLU */
    int oldLogNumbers = searchInFile(LOG_FILE, searchedFileName, 'l');

    fpLogFile = fopen(LOG_FILE, "a+");
    if (NULL == fpLogFile) {
        fprintf(stderr, "%s\n","Cannot open/create logFile. Check program.!!!");
        exit(1);
    }
    fprintf(fpLogFile, "\n-> %s ## %d ##\n",searchedFileName,oldLogNumbers + 1);

    return fpLogFile;
}

int findOccurencesInLine(const char *line, int startPoint, const char *word) {

    int indexOfLine = 0;
    int columnNumber = 0;
    int i = 0;
    int j = 0;

    /*Istenilen indisten itibaren kelime aramasi yapar */
    for (i = startPoint; i < strlen(line); ++i) {
        indexOfLine = i;
        columnNumber = i;

        for (j = 0; j < strlen(word); ++j) {
            if (line[indexOfLine] == word[j]) {
                ++indexOfLine;
                if (j == strlen(word) - 1) {
                    return columnNumber;
                }
            } else break;
        }
    }
    return -1; /* Kelime bulunamadi*/
}

int searchInFile(const char *fileName, const char *word, char logOrNormal) {

    FILE *fpWordFile;
    FILE *fpLogFile;
    const unsigned char NEW_LINE = '\n';
    int lineLength = 0;
    int lineNumber = 0;
    int totalOccurence = 0;
    char *line = NULL;
    char ch;
    int columnNumber;
    bool logCreated = false;

    if (NULL == (fpWordFile = fopen(fileName, "r"))) {
        fprintf(stderr, "File \"%s\" can't found in directory.\n", fileName);
        return -1;
    }


    do {
        columnNumber = 0;
        lineLength = 0;
        /* Satir sonuna kadar oku ve satirdaki harf sayisini bul*/
        for (ch = fgetc(fpWordFile);
                ch != NEW_LINE && ch != EOF;
                ch = fgetc(fpWordFile), ++lineLength) {
        }
        if (ch != EOF) {
            /* Dosya okuma imlecini asil okuma icin satir basina al*/
            fseek(fpWordFile, -lineLength - 1, SEEK_CUR);

            free(line);
            line = (char *) calloc(sizeof (char), lineLength + 1);
            /*Tum satiri oku*/
            fgets(line, lineLength + 1, fpWordFile);
            /* char * oldugu icin sonuna null koy */
            line[lineLength] = '\0';

            /* Satir icinde kelime ara ve bulunma durumlarında log file acip
             * log file icine yaz */
            /* LOG MODUNDA ACILIRSA SADECE KELIMELERI SAY */
            while (-1 != (columnNumber =
                            findOccurencesInLine(line, columnNumber, word))) {
                if (logOrNormal == 'n') {
                    if (!logCreated) {
                        fpLogFile = createLogFile(fileName);
                        logCreated = true;
                    }
                    fprintf(fpLogFile, "%d. %s %d %d\n", totalOccurence + 1,
                                        word, lineNumber, columnNumber);
                }
                ++totalOccurence;
                ++columnNumber;
            }
        }

        ++lineNumber;
        /*Dosya sonunda EOF ile ulasip olusturulan bellekleri heap e geri ver*/
        ch = fgetc(fpWordFile);

    } while (ch != EOF);

    /*LOG DOSYASI ACILDIYSA KAPA*/
    if (logCreated)
        fclose(fpLogFile);

    /* Dinamik alanlarin silinmesi */
    free(line);
    fclose(fpWordFile);
    return totalOccurence;
}
