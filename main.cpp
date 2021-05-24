#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <array>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <malloc.h>
#define LIMITE 4
using namespace std;
struct nodo
{
    struct nodo *der,*izq,*arr;  /* forma el nodo */
    int cuenta;                  /* apariciones del carácter */
    char bit;                    /* 0 o 1 */
    unsigned char karacter;      /* el carácter (para la descompresión */
    char *codigo;                /* cadena de ceros y unos con la codificación */
    char nbits;                  /* me apunto el numero de bits que codifican el carácter */
}HOJAS[256],*TELAR[256],*MENOR,*SEGUNDO;

int NSIMB=0,nsimb;
FILE *f,*g;
int NBYTES=0;
char *tabla [256];

/*--------------------------------
preparar las hojas
--------------------------------*/
int preparar_hojas(char *archivo)
{
    int j;
    for(j=0;j<256;++j){
        HOJAS[j].der=HOJAS[j].izq=HOJAS[j].arr=NULL;
        HOJAS[j].cuenta=0;
        HOJAS[j].karacter=j;
        HOJAS[j].codigo=NULL;
    }
    if ((f=fopen(archivo,"rb"))!=NULL){
        while ((j=fgetc(f))!=EOF){
            ++HOJAS[j].cuenta;
            ++NBYTES;
        }
        fclose(f);
    }
    else
    {
        return(1);
    }
    for(j=0;j<256;++j){
        if (HOJAS[j].cuenta!=0)
            ++NSIMB;
    }
    nsimb=NSIMB;
    return(0);
}

/*--------------------------------
preparar telar
--------------------------------*/
void preparar_telar()
{
    int j;
    for(j=0;j<256;++j){
        TELAR[j]=&(HOJAS[j]);
    }
    return;
}

/*--------------------------------
tejer el árbol
--------------------------------*/
void tejer()
{
    int menor=-1;     /* guarda indice */
    int segundo=-1;   /* guarda indice */
    int temporal;     /* guarda cuenta */
    int j;
    struct nodo *P;   /* nuevo nodo */

    if (nsimb==1) return;

    /* buscar menor valor */
    for(j=0;j<256;++j){
        if (TELAR[j]==NULL) continue;
        if (TELAR[j]->cuenta==0) continue;
        if (menor==-1){
            menor=j;
            temporal=TELAR[j]->cuenta;
        } else
        {
            if (TELAR[j]->cuenta<temporal){
                menor=j;
                temporal=TELAR[j]->cuenta;
            }
        }
    }

    /* buscar segundo menor */
    for(j=0;j<256;++j){
        if (TELAR[j]==NULL) continue;
        if (TELAR[j]->cuenta==0) continue;
        if (j==menor) continue;
        if (segundo==-1){
            segundo=j;
            temporal=TELAR[j]->cuenta;
        } else
        {
            if (TELAR[j]->cuenta<temporal){
                segundo=j;
                temporal=TELAR[j]->cuenta;
            }
        }
    }

    /* tejer un nuevo nodo */
    P=(struct nodo *)malloc(sizeof(struct nodo));
    TELAR[menor]->arr=P;
    TELAR[segundo]->arr=P;
    P->izq=TELAR[menor];
    P->der=TELAR[segundo];
    P->arr=NULL;
    TELAR[menor]->bit=0;
    TELAR[segundo]->bit=1;
    P->cuenta=TELAR[menor]->cuenta+TELAR[segundo]->cuenta;
    TELAR[menor]=NULL;
    TELAR[segundo]=P;
    --nsimb;

    /* sigue tejiendo hasta que sólo quede un nodo */
    tejer();
}

/*--------------------------------
Una vez construido el árbol, puedo codificar
cada carácter. Para eso recorro desde la hoja
a la raíz, apunto 0 o 1 en una pila y luego
paso la pila a una cadena. Un 2 determina el
fin de la cadena.
--------------------------------*/
void codificar()
{
    char pila[64];
    char tope;
    int j;
    char *w;
    struct nodo *P;
    for(j=0;j<256;++j){
        if (HOJAS[j].cuenta==0) continue;
        P=(struct nodo *)(&(HOJAS[j]));
        tope=0;
        while (P->arr!=NULL){
            pila[tope]=P->bit;
            ++tope;
            P=P->arr;
        }
        HOJAS[j].nbits=tope;
        HOJAS[j].codigo=(char *)malloc((tope+1)*sizeof(char));
        w=HOJAS[j].codigo;
        --tope;
        while (tope>-1){
            *w=pila[tope];
            --tope;
            ++w;
        }
        *w=2;
    }
    return;
}


/*--------------------------------
debug. Imprime la info sobre cada
carácter, como número de apariciones
y cadena con que se codifica
--------------------------------*/
void debug()
{
    int j,k;
    char *w;
    int tam_comprimido=0;
    for(j=0;j<256;++j){
        if (HOJAS[j].cuenta==0) continue;
        tam_comprimido+=(HOJAS[j].cuenta*HOJAS[j].nbits);
        printf("%3d %6d ",j,HOJAS[j].cuenta);
        w=HOJAS[j].codigo;
        while (*w!=2){
            printf("%c",48+(*w));
            ++w;
        }
        printf("\n");
    }
    printf("NSIMB: %d\n",NSIMB);
    printf("NBYTES: %d\n",NBYTES);
    printf("TAMAÑO COMPRIMIDO: %d\n",tam_comprimido/8+1);
    return;
}

/*--------------------------------
Escribe la cabecera del archivo de
destino. La cabecera contiene: el
número de bytes del archivo origen,
el número de caracteres distintos
en ese archivo y una lista de parejas
número de carácter-cuenta de ese
carácter. Eso es suficiente para la
descompresión
--------------------------------*/
int escribe_cabecera(char *destino)
{
    int j,k;
    FILE *g;

    char *p=(char *)(&NBYTES);
    if ((g=fopen(destino,"wb"))==NULL) return(1);
    for(j=0;j<4;++j){
        fputc(*p,g);
        ++p;
    }

    p=(char *)(&NSIMB);
    fputc(*p,g);

    for(j=0;j<256;++j){
        if (HOJAS[j].cuenta==0) continue;
        fputc(j,g);
        p=(char *)(&(HOJAS[j].cuenta));
        for(k=0;k<4;++k){
            fputc(*p,g);
            ++p;
        }
    }
    fclose(g);
    return(0);
}

/*--------------------------------
Una vez construido el árbol y codificado
cada carácter se puede proceder a la
compresión: se tomará carácter a carácter
del archivo origen y se usará la cadena
de codificación para ir escribiendo
bits en un buffer de un carácter, que
cada vez que quede lleno se pasará al
archivo de destino
--------------------------------*/
int comprimir(char *origen, char *destino)
{
    unsigned char d=0;
    int x;
    char nbit=0;
    char *p;

    if ((f=fopen(origen,"rb"))==NULL) return(1);
    if ((g=fopen(destino,"ab"))==NULL) return(2); /* ya esta la cabecera */

    while ((x=fgetc(f))!=EOF){
        p=HOJAS[x].codigo;
        while (*p!=2){
            if (nbit==8){
                nbit=0;
                fputc(d,g);
                d=0;
            } else
            {
                if (*p==1){
                    d|=(1<<nbit);
                }
                ++nbit;
                ++p;
            }
        }
    }
    fputc(d,g);
    fclose(f);
    fclose(g);
    return(0);
}

/*--------------------------------
Descomprime el archivo. El primer paso
es leer la cabecera, paso previo a la
descompresión. Recuerdo formato de
la cabecera:
NBYTES|NSIMB|(char,cuenta)*
--------------------------------*/
int descomprimir(char *origen, char *destino)
{
    char *p;
    int j,k,n,m;
    unsigned char x,nbit;
    struct nodo *P,*Q;

    if ((g=fopen(origen,"rb"))==NULL) return(1);
    if ((f=fopen(destino,"wb"))==NULL) return(2);

    /* leer NBYTES */
    p=(char *)(&n);
    for(j=0;j<4;++j){
        *p=(unsigned char)fgetc(g);
        ++p;
    }
    NBYTES=n;

    /* leer NSIMB */
    NSIMB=nsimb=fgetc(g);

    /* preparar las hojas */
    for(j=0;j<256;++j){
        HOJAS[j].cuenta=0;
        HOJAS[j].izq=HOJAS[j].der=HOJAS[j].arr=NULL;
        HOJAS[j].karacter=j;
    }
    for(j=0;j<NSIMB;++j){
        n=fgetc(g);
        p=(char *)(&m);
        for(k=0;k<4;++k){
            *p=(unsigned char)fgetc(g);
            ++p;
        }
        HOJAS[n].cuenta=m;
    }

    /* construyo el árbol */
    preparar_telar();
    tejer();

    /* apunto a la raíz del árbol */
    j=0;
    while (HOJAS[j].cuenta==0) ++j;
    P=(struct nodo *)(&(HOJAS[j]));
    while (P->arr!=NULL) P=P->arr;

    /* ahora ya se puede descomprimir */
    j=0;
    x=fgetc(g);
    nbit=0;
    Q=P;
    while(j<NBYTES){
        if (Q->izq==NULL){
            fputc(Q->karacter,f);
            Q=P;
            ++j;
        } else
        if (nbit==8){
            x=fgetc(g);
            nbit=0;
        } else
        {
            if (x&(1<<nbit)){
                Q=Q->der;
            }
            else
            {
                Q=Q->izq;
            }
            ++nbit;
        }
    }
    fclose(f);
    fclose(g);
    return(0);
}

void printPermutations(char *str, char* permutations, int last, int index){
    ofstream salida("genoma.dat",ios::app);/*abre fichero con opcion app*/
    salida.clear();/*borra fichero*/
     int i, len = strlen(str);
   for (i = 0; i < len; i++ ) {
      permutations[index] = str[i] ;
      if (index == last) {
          permutations[last+1] = '\0';/*ajuste para que no de 'basura'*/
          cout<<permutations <<"\t";/*saca por pantalla los valores de la permutacion*/
          salida<<permutations<<" "<<endl; /*<< "\n" o << "\t"*/;/*escribe en fichero los valores de la permutacion*/
         /* strcpy(tabla[i],permutations);*/

      }
      else {
          printPermutations (str, permutations, last, index+1);
         };}
         salida.close();}/*cierra fichero*/
void compresor(){
    ifstream entrada;
    ifstream genoma;
    ofstream comprimido("comprimido.dat",ios::app);
    string dato,dat3,dat8[256];
    entrada.open("entrada.dat");
    genoma.open("genoma.dat");
    if(!entrada | !genoma)
    cout << "Error abriendo el fichero" << endl;
    else
    {  int i=0;
        while(genoma>>dat3){
            dat8[i]==dat3;
            i++;
        };
        while(entrada>>dato){
                cout << dato << endl;
                int j=0;
                do {
                   if (dato.compare(dat8[j]) != 0) {comprimido<<j;}
                    j++;
                       }while(j<256);

            };
            entrada.close();
            genoma.close();
            comprimido.close();
}
};
void descompresor(){
    ifstream genoma;
    ifstream comprimido("comprimido.dat",ios::app);
    ofstream salida("salida.dat",ios::app);
    string dato,dat3,dat8[256];
    genoma.open("genoma.dat");
    if(!comprimido || !genoma || !salida)
    cout << "Error abriendo el fichero" << endl;
    else
    {  int i=0;
        while(genoma>>dat3){
            dat8[i]==dat3;
            i++;
        };
        { /* descomprimir */
                          if (descomprimir("comprimido.tgz","comprimido.dat")){
                              printf("error abriendo archivo\n");
                                               }

                            preparar_telar();
                            tejer();
                            codificar();
                            debug();
                                };
        while(comprimido>>dato){
                cout << dato << endl;
                int j=0;
                do {
                   if (dato.compare(dat8[j]) != 0) {salida<<j;}
                    j++;
                       }while(j<256);

            };
            genoma.close();
            comprimido.close();
}
};
int main(int argc, char *argv[])/*menu*/
{   int j;
    int opcion;
    do{
        cout << "\n1.- Generar combinaciones y archivo genoma.dat /Build combinations & genoma.dat file"
             << "\n2.- comprimir HUFFMAN genoma.dat /compress HUFFMAN genoma.dat"
             << "\n3.- descomprimir HUFFMAN genoma.dat/decompress HUFFMAN genoma.dat"
             <<"\n4.- compresion propia/own compression"
             <<"\n5.-descompresion propia/decompression own"
             << "\n0.- Salir/Exit"
             << "\nOPCION: /OPTION:";/*opciones menu*/
        cin >> opcion;

        switch ( opcion ) {

            case 1:{
            char str[] = "ATGC";/*inicializa con valor de Amina Tiamina Guamina y Citosina*/
            cout<<"Todas las permutaciones,CON REPETICION,de/All permutation with REPETION "<<str<<" son:/are: "<<endl ;/*muestra el mensaje para dar por pantalla todos los valores de las permutaciones*/
            int len = strlen(str) ;
            char permutations[len];
            printPermutations (str, permutations, len-1, 0);/*llama a la funcion*/
                   }
                break;

            case 2:{ /* comprimir */
                    if (preparar_hojas("genoma.dat")){
                    printf("error abriendo archivo/Error opening file \n");
                    return 0;
                    }
                    preparar_telar();
                    tejer();
                    codificar();
                    if (escribe_cabecera("genoma.tgz")){
                    printf("error abriendo archivo/Error opening file \n");
                    return 0;
                    }
                    if (comprimir("genoma.dat","genoma.tgz")){
                    printf("error abriendo archivo/Error opening file \n");
                    return 0;
                    }
                            /*comprimir("genoma.dat","genoma.tgz");*/
                            };
                break;

            case 3:{ /* descomprimir */
                          if (descomprimir("genoma.tgz","genoma.dat")){
                              printf("error abriendo archivo/Error opening file \n");
                              return 0;
                            }

                            preparar_telar();
                            tejer();
                            codificar();
                            debug();
                                };

                break;
            case 4:{compresor();
                    if (preparar_hojas("comprimido.dat")){
                    printf("error abriendo archivo/Error opening file \n");
                    return 0;
                    }
                    preparar_telar();
                    tejer();
                    codificar();
                    if (escribe_cabecera("comprimido.tgz")){
                    printf("error abriendo archivo/Error opening file \n");
                    return 0;
                    }
                    if (comprimir("comprimido.dat","comprimido.tgz")){
                    printf("error abriendo archivo/Error opening file \n");
                    return 0;};
                    };
                break;
            case 5:{descompresor();};
                break;
            case 0:
                cout << "\nFIN DEL PROGRAMA/END OF PROGRAM" << endl;/*fin del programa*/
                break;

            default:
                break;
        }

    } while( opcion != 0 );

    return 0;
}

