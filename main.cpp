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
using namespace std;
/* Tipo nodo para �rbol o Lista de �rboles*/
/* El prop�sito es dual, sirve como elemento de una lista enlazada */
/* Cuando se usa el puntero sig, y como elemento de un �rbol cuando */
/* se usan los punteros cero y uno */
typedef struct _nodo
{
   char letra;      /* Letra a la que hace referencia el nodo */
   int frecuencia;  /* veces que aparece la letra en el texto o las letras */
                    /* de los nodos de las ramas cero y uno */
   _nodo *sig;      /* Puntero a siguiente nodo de una lista enlazada */
   _nodo *cero;     /* Puntero a la rama cero de un �rbol */
   _nodo *uno;      /* Puntero a la rama uno de un �rbol */
} tipoNodo;         /* Nombre de tipo */

/* Nodo para construir una lista para la tabla de codigos */
typedef struct _tabla
{
   char letra;      /* Letra a la que hace referencia el nodo */
   unsigned long int bits; /* Valor de la codificaci�n de la letra */
   char nbits;      /* N�mero de bits de la codificaci�n */
   _tabla *sig;     /* Siguiente elemento de la tabla */
} tipoTabla;        /* Nombre del tipo */

/* Variables globales */
tipoTabla *Tabla;

/* Prototipos */
void Cuenta(tipoNodo* &Lista, char c);
void Ordenar(tipoNodo* &Lista);
void InsertarOrden(tipoNodo* &Cabeza, tipoNodo *e);
void BorrarArbol(tipoNodo *n);
void CrearTabla(tipoNodo *n, int l, int v);
void InsertarTabla(char c, int l, int v);
tipoTabla *BuscaCaracter(tipoTabla *Tabla, char c);

int comprimir(int argc, char *argv[])
{
   tipoNodo *Lista;       /* Lista de letras y frecuencias */
   tipoNodo *Arbol;       /* Arbol de letras y frecuencias */

   FILE *fe, *fs;         /* Ficheros de entrada y salida */
   char c;                /* variables auxiliares */
   tipoNodo *p;
   tipoTabla *t;
   int nElementos;        /* N�mero de elementos en tabla */
   long int Longitud = 0; /* Longitud del fichero original */

   unsigned long int dWORD; /* Soble palabra usada durante la codificaci�n */
   int nBits;               /* N�mero de bits usados de dWORD */

   if(argc < 3)
   {
      printf("Usar:\n%s <genoma.dat> <genoma.tgz>\n", argv[0]);
      return 1;
   }

   Lista = NULL;
   /* Fase 1: contar frecuencias */
   fe = fopen("genoma.dat","r+");
   while((c = fgetc(fe)) != EOF)
   {
      Longitud++;       /* Actualiza la cuenta de la longitud del fichero */
      Cuenta(Lista, c); /* Actualiza la lista de frecuencias */
   }
   fclose(fe);

   /* Ordenar la lista de menor a mayor */
   Ordenar(Lista);

   /* Crear el arbol */
   Arbol = Lista;
   while(Arbol && Arbol->sig) /* Mientras existan al menos dos elementos en la lista */
   {
      p = (tipoNodo *)malloc(sizeof(tipoNodo)); /* Un nuevo �rbol */
      p->letra = 0;                             /* No corresponde a ninguna letra */
      p->uno = Arbol;                           /* Rama uno */
      Arbol = Arbol->sig;                       /* Siguiente nodo en */
      p->cero = Arbol;                          /* Rama cero */
      Arbol = Arbol->sig;                       /* Siguiente nodo */
      p->frecuencia = p->uno->frecuencia +
                      p->cero->frecuencia;      /* Suma de frecuencias */
      InsertarOrden(Arbol, p);                  /* Inserta en nuevo nodo */
   }                                            /* orden de frecuencia */

   /* Construir la tabla de c�digos binarios */
   Tabla = NULL;
   CrearTabla(Arbol, 0, 0);

   /* Crear fichero comprimido */
   fs = fopen("genoma.tgz","w+");
   /* Escribir la longitud del fichero */
   fwrite(&Longitud, sizeof(long int), 1, fs);
   /* Cuenta el n�mero de elementos de tabla */
   nElementos = 0;
   t = Tabla;
   while(t)
   {
      nElementos++;
      t = t->sig;
   }
   /* Escribir el n�mero de elemenos de tabla */
   fwrite(&nElementos, sizeof(int), 1, fs);
   /* Escribir tabla en fichero */
   t = Tabla;
   while(t)
   {
      fwrite(&t->letra, sizeof(char), 1, fs);
      fwrite(&t->bits, sizeof(unsigned long int), 1, fs);
      fwrite(&t->nbits, sizeof(char), 1, fs);
      t = t->sig;
   }

   /* Codificaci�n del fichero de entrada */
   fe = fopen(argv[1], "genoma.dat");
   dWORD = 0; /* Valor inicial. */
   nBits = 0; /* Ning�n bit */
   while((c = fgetc(fe)) != EOF)
   {
      /* Busca c en tabla: */
      t = BuscaCaracter(Tabla, c);
      /* Si nBits + t->nbits > 32, sacar un byte */
      while(nBits + t->nbits > 32)
      {
         c = dWORD >> (nBits-8);           /* Extrae los 8 bits de mayor peso */
         fwrite(&c, sizeof(char), 1, fs);  /* Y lo escribe en el fichero */
         nBits -= 8;                       /* Ya tenemos hueco para 8 bits m�s */
      }
      dWORD <<= t->nbits; /* Hacemos sitio para el nuevo caracter */
      dWORD |= t->bits;   /* Insertamos el nuevo caracter */
      nBits += t->nbits;  /* Actualizamos la cuenta de bits */
   }
   /* Extraer los cuatro bytes que quedan en dWORD*/
   while(nBits>0)
   {
      if(nBits>=8) c = dWORD >> (nBits-8);
      else c = dWORD << (8-nBits);
      fwrite(&c, sizeof(char), 1, fs);
      nBits -= 8;
   }

   fclose(fe);  /* Cierra los ficheros */
   fclose(fs);

   /* Borrar Arbol */
   BorrarArbol(Arbol);

   /* Borrar Tabla */
   while(Tabla)
   {
      t = Tabla;
      Tabla = t->sig;
      free(t);
   }

   return 0;
}

/* Actualiza la cuenta de frecuencia del car�cter c */
/* El puntero a Lista se pasa por referencia, ya que debe poder cambiar */
/* Ya sea por que la lista est� vac�a, o porque el nuevo elemento sea el */
/* primero */
void Cuenta(tipoNodo* &Lista, char c)
{
   tipoNodo *p, *a, *q;

   if(!Lista)  /* Si la lista est� vac�a, el nuevo nodo ser� Lista */
   {
      Lista = (tipoNodo *)malloc(sizeof(tipoNodo)); /* Un nodo nuevo */
      Lista->letra = c;                             /* Para c */
      Lista->frecuencia = 1;                        /* en su 1� aparici�n */
      Lista->sig = Lista->cero = Lista->uno = NULL;
   }
   else
   {
      /* Buscar el caracter en la lista (ordenada por letra) */
      p = Lista;
      a = NULL;
      while(p && p->letra < c)
      {
         a = p;      /* Guardamos el elemento actual para insertar */
         p = p->sig; /* Avanzamos al siguiente */
      }
      /* Dos casos: */
      /* 1) La letra es c se encontr� */
      if(p && p->letra == c) p->frecuencia++; /* Actualizar frecuencia */
      else
      /* 2) La letra c no se encontr�*/
      {
         /* Insertar un elemento nuevo */
         q = (tipoNodo *)malloc(sizeof(tipoNodo));
         q->letra = c;
         q->frecuencia = 1;
         q->cero = q->uno = NULL;
         q->sig = p;        /* Insertar entre los nodos p */
         if(a) a->sig = q;  /* y a */
         else Lista = q;    /* Si a es NULL el nuevo es el primero */
      }
   }
}

/* Ordena Lista de menor a mayor por frecuencias */
/* De nuevo pasamos el puntero a lista por referencia */
void Ordenar(tipoNodo* &Lista)
{
   tipoNodo *Lista2, *a;

   if(!Lista) return; /* Lista vacia */
   Lista2 = Lista;
   Lista = NULL;
   while(Lista2)
   {
      a = Lista2;              /* Toma los elementos de Lista2 */
      Lista2 = a->sig;
      InsertarOrden(Lista, a); /* Y los inserta por orden en Lista */
   }
}

/* Inserta el elemento e en la Lista ordenado por frecuencia de menor a mayor */
/* El puntero a Cabeza se pasa por referencia */
void InsertarOrden(tipoNodo* &Cabeza, tipoNodo *e)
{
   tipoNodo *p, *a;

   if(!Cabeza) /* Si Cabeza en NULL, e es el primer elemento */
   {
      Cabeza = e;
      Cabeza->sig = NULL;
   }
   else
   {
       /* Buscar el caracter en la lista (ordenada por frecuencia) */
       p = Cabeza;
       a = NULL;
       while(p && p->frecuencia < e->frecuencia)
       {
          a = p;      /* Guardamos el elemento actual para insertar */
          p = p->sig; /* Avanzamos al siguiente */
       }
       /* Insertar el elemento */
       e->sig = p;
       if(a) a->sig = e;   /* Insertar entre a y p */
       else Cabeza = e;    /* el nuevo es el primero */
    }
}

/* Funci�n recursiva para crear Tabla */
/* Recorre el �rbol cuya raiz es n y le asigna el c�digo v de l bits */
void CrearTabla(tipoNodo *n, int l, int v)
{
   if(n->uno)  CrearTabla(n->uno, l+1, (v<<1)|1);
   if(n->cero) CrearTabla(n->cero, l+1, v<<1);
   if(!n->uno && !n->cero) InsertarTabla(n->letra, l, v);
}

/* Insertar un elemento en la tabla */
void InsertarTabla(char c, int l, int v)
{
   tipoTabla *t, *p, *a;

   t = (tipoTabla *)malloc(sizeof(tipoTabla)); /* Crea un elemento de tabla */
   t->letra = c;                               /* Y lo inicializa */
   t->bits = v;
   t->nbits = l;

   if(!Tabla)         /* Si tabla es NULL, entonces el elemento t es el 1� */
   {
      Tabla = t;
      Tabla->sig = NULL;
   }
   else
   {
       /* Buscar el caracter en la lista (ordenada por frecuencia) */
       p = Tabla;
       a = NULL;
       while(p && p->letra < t->letra)
       {
          a = p;      /* Guardamos el elemento actual para insertar */
          p = p->sig; /* Avanzamos al siguiente */
       }
       /* Insertar el elemento */
       t->sig = p;
       if(a) a->sig = t;  /* Insertar entre a y p */
       else Tabla = t;    /* el nuevo es el primero */
    }
}

/* Buscar un caracter en la tabla, devielve un puntero al elemento de la tabla */
tipoTabla *BuscaCaracter(tipoTabla *Tabla, char c)
{
   tipoTabla *t;

   t = Tabla;
   while(t && t->letra != c) t = t->sig;
   return t;
}

/* Funci�n recursiva para borrar un arbol */
void BorrarArbol(tipoNodo *n)
{
   if(n->cero) BorrarArbol(n->cero);
   if(n->uno)  BorrarArbol(n->uno);
   free(n);
}
/* Tipo nodo para �rbol */
typedef struct _noda
{
   char letras;             /* letras a la que hace referencia el nodo */
   unsigned long int bitios; /* Valor de la codificaci�n de la letras */
   char nbitios;             /* N�mero de bitios de la codificaci�n */
   _noda *cero;            /* Puntero a la rama cero de un �rbol */
   _noda *uno;             /* Puntero a la rama uno de un �rbol */
} tipoNoda;                /* Nombre del tipo */

/* Funciones prototipo */
void BorrarArbolito(tipoNoda *n);

int descomprimir(int argz, char *argt[])
{
   tipoNoda *Arbolito;        /* Arbolito de codificaci�n */
   long int Largo;      /* Largo de fichero */
   int nElementas;         /* Elementos de �rbol */
   unsigned long int bitios; /* Almacen de bitios para decodificaci�n */
   FILE *fe, *fs;          /* Ficheros de entrada y salida */

   tipoNoda *p, *q;        /* Auxiliares */
   unsigned char a;
   int i, j;

   if(argz < 3)
   {
      printf("Usar:\n%s <genoma.tgz> <genoma.dat>\n", argt[0]);
      return 1;
   }

   /* Crear un Arbolito con la informaci�n de la tabla */
   Arbolito = (tipoNoda *)malloc(sizeof(tipoNoda)); /* un nodo nuevo */
   Arbolito->letras = 0;
   Arbolito->uno = Arbolito->cero = NULL;
   fe = fopen("genoma.tgz","r+");
   fread(&Largo, sizeof(long int), 1, fe); /* Lee el n�mero de caracteres */
   fread(&nElementas, sizeof(int), 1, fe); /* Lee el n�mero de elementos */
   for(i = 0; i < nElementas; i++) /* Leer todos los elementos */
   {
      p = (tipoNoda *)malloc(sizeof(tipoNoda)); /* un nodo nuevo */
      fread(&p->letras, sizeof(char), 1, fe); /* Lee el car�cter */
      fread(&p->bitios, sizeof(unsigned long int), 1, fe); /* Lee el c�digo */
      fread(&p->nbitios, sizeof(char), 1, fe); /* Lee la Largo */
      p->cero = p->uno = NULL;
      /* Insertar el nodo en su lugar */
      j = 1 << (p->nbitios-1);
      q = Arbolito;
      while(j > 1)
      {
         if(p->bitios & j) /* es un uno*/
            if(q->uno) q = q->uno;   /* Si el nodo existe, nos movemos a �l */
            else                     /* Si no existe, lo creamos */
            {
               q->uno = (tipoNoda *)malloc(sizeof(tipoNoda)); /* un nodo nuevo */
               q = q->uno;
               q->letras = 0;
               q->uno = q->cero = NULL;
            }
         else /* es un cero */
            if(q->cero) q = q->cero; /* Si el nodo existe, nos movemos a �l */
            else                     /* Si no existe, lo creamos */
            {
               q->cero = (tipoNoda *)malloc(sizeof(tipoNoda)); /* un nodo nuevo */
               q = q->cero;
               q->letras = 0;
               q->uno = q->cero = NULL;
            }
         j >>= 1;  /* Siguiente bit */
      }
      /* Ultimo Bit */
      if(p->bitios & 1) /* es un uno*/
         q->uno = p;
      else            /* es un cero */
         q->cero = p;
   }
   /* Leer datos comprimidos y extraer al fichero de salida */
   bitios = 0;
   fs = fopen("genoma.dat","w+");
   /* Lee los primeros cuatro bytes en la doble palabra bitios */
   fread(&a, sizeof(char), 1, fe);
   bitios |= a;
   bitios <<= 8;
   fread(&a, sizeof(char), 1, fe);
   bitios |= a;
   bitios <<= 8;
   fread(&a, sizeof(char), 1, fe);
   bitios |= a;
   bitios <<= 8;
   fread(&a, sizeof(char), 1, fe);
   bitios |= a;
   j = 0; /* Cada 8 bitios leemos otro byte */
   q = Arbolito;
   /* Bucle */
   do {
      if(bitios & 0x80000000) q = q->uno; else q = q->cero; /* Rama adecuada */
      bitios <<= 1;           /* Siguiente bit */
      j++;
      if(8 == j)            /* Cada 8 bitios */
      {
         i = fread(&a, sizeof(char), 1, fe); /* Leemos un byte desde el fichero */
         bitios |= a;                    /* Y lo insertamos en bitios */
         j = 0;                        /* No quedan huecos */
      }
      if(!q->uno && !q->cero)          /* Si el nodo es una letras */
      {
         putc(q->letras, fs);           /* La escribimos en el fich de salida */
         Largo--;                   /* Actualizamos Largo que queda */
         q=Arbolito;                      /* Volvemos a la raiz del �rbol */
      }
   } while(Largo);                  /* Hasta que acabe el fichero */
   /* Procesar la cola */

   fclose(fs);                         /* Cerramos ficheros */
   fclose(fe);

   BorrarArbolito(Arbolito);                 /* Borramos el �rbol */
   return 0;
}

/* Funci�n recursiva para borrar un Arbolito */
void BorrarArbolito(tipoNoda *n)
{
   if(n->cero) BorrarArbolito(n->cero);
   if(n->uno)  BorrarArbolito(n->uno);
   free(n);
}

void printPermutations(char *str, char* permutations, int last, int index){
    ofstream salida("genoma.dat",ios::app);/*abre fichero con opcion app*/
    salida.clear();/*borra fichero*/
    int i, len = strlen(str);
   for ( i = 0; i < len; i++ ) {
      permutations[index] = str[i] ;
      if (index == last) {
          permutations[last+1] = '\0';/*ajuste para que no de 'basura'*/
          cout<<permutations <<"\t";/*saca por pantalla los valores de la permutacion*/
          salida<<permutations /*<< "\n" o << "\t"*/;/*escribe en fichero los valores de la permutacion*/
      }
      else {
          printPermutations (str, permutations, last, index+1);
         }}
         salida.close();}/*cierra fichero*/

int main(int argm, char *argn[])/*menu*/
{   int opcion;
    do{
        cout << "\n1.- Generar combinaciones y archivo genoma.dat "
             << "\n2.- comprimir"
             << "\n3.- descomprimir"
             << "\n4.- Salir"
             << "\nOPCION: ";/*opciones menu*/
        cin >> opcion;

        switch ( opcion ) {

            case 1:{
            char str[] = "ATGC";/*inicializa con valor de Amina Tiamina Guamina y Citosina*/
            cout<<"Todas las permutaciones,CON REPETICION,de "<<str<<" son: "<<endl ;/*muestra el mensaje para dar por pantalla todos los valores de las permutaciones*/
            int len = strlen(str) ;
            char permutations[len];
            printPermutations (str, permutations, len-1, 0);/*llama a la funcion*/
                   }
                break;

            case 2:{argm=1;
                    comprimir(argm,&*argn);}/*llama a comprimir pasando valores*/

                break;

            case 3:{argm=4;
                    descomprimir(argm,&*argn);}/*llama a comprimir pasando valores*/

                break;

            case 4:
                cout << "\nFIN DEL PROGRAMA" << endl;/*fin del programa*/
                break;

            default:
                break;
        }

    } while( opcion != 4 );

    return 0;
}

    /*char cadena[128];
   // Crea un fichero de salida
   /*ofstream fs("genoma.tgz");

   // Enviamos una cadena al fichero de salida:
   /*fs << "Hola, mundo" << endl;
   // Cerrar el fichero,
   // para luego poder abrirlo para lectura:
   fs.close();

   // Abre un fichero de entrada
   ifstream fe("genoma.tgz");

   // Leeremos mediante getline, si lo hici�ramos
   // mediante el operador << s�lo leer�amos
   // parte de la cadena:
   fe.getline(cadena, 128);

   cout << cadena << endl;
    return 0;
*/

