/* reads a locale message catalog created with gencat */ 

#include <nl_types.h>
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <strings.h>


int main(int argc,char *argv[]) {
    nl_catd catd = NULL;
    int idx;
    char fncatalog[64] = "SUNWsamfs";
    
    (void) setlocale(LC_ALL, "C");
    
    if (argc == 2)
        strncpy(fncatalog, argv[1],64);
    
    catd = catopen(fncatalog, NL_CAT_LOCALE);
    if (catd == (nl_catd)(-1)) {
        fprintf(stderr,"catalog not found\n");
        return (1);
    }
    if (catd->__size == 0) {
        fprintf(stderr,"catalog empty\n");
        return (1);
    }
    
    for (idx = 30000; idx< 31300; idx++) {
        char *msg = catgets(catd, 1, idx, "");
        if ((*msg) != '\0') 
            printf("%d %s\n", idx, msg);
    }
    catclose(catd);
}

