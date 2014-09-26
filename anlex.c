/******************** LIbrerias *******************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>

/***************** Constantes **********************/

#define TAMBUFF 	5
#define TAMLEX 		6
#define TAMHASH 	101

/************* Definiciones ********************/

typedef struct entrada{
	//definir los campos de 1 entrada de la tabla de simbolos
	char *compLex;
	char lexema[TAMLEX];	
	struct entrada *tipoDato; // null puede representar variable no declarada	
	// aqui irian mas atributos para funciones y procedimientos...
	
} entrada;

typedef struct {
	char *compLex;
	entrada *pe;
} token;

/************* Variables globales **************/

int consumir;			/* 1 indica al analizador lexico que debe devolver
						el sgte componente lexico, 0 debe devolver el actual */

char cad[5*TAMLEX];		// string utilizado para cargar mensajes de error
token t;				// token global para recibir componentes del Analizador Lexico

// variables para el analizador lexico

FILE *archivo;			// Fuente Json
FILE *archSalida;		// Archivo de salida
char buff[2*TAMBUFF];	// Buffer para lectura de archivo fuente
char id[TAMLEX];		// Utilizado por el analizador lexico
int delantero=-1;		// Utilizado por el analizador lexico
int fin=0;				// Utilizado por el analizador lexico
int numLinea=1;			// Numero de Linea

/************** Prototipos *********************/


void sigLex();		// Del analizador Lexico

/**************** Funciones **********************/

/*********************HASH************************/
entrada *tabla;				//declarar la tabla de simbolos
int tamTabla=TAMHASH;		//utilizado para cuando se debe hacer rehash
int elems=0;				//utilizado para cuando se debe hacer rehash

int h(const char* k, int m)
{
	unsigned h=0,g;
	int i;
	for (i=0;i<strlen(k);i++)
	{
		h=(h << 4) + k[i];
		if (g=h&0xf0000000){
			h=h^(g>>24);
			h=h^g;
		}
	}
	return h%m;
}
void insertar(entrada e);

void initTabla()
{	
	int i=0;
	
	tabla=(entrada*)malloc(tamTabla*sizeof(entrada));
	for(i=0;i<tamTabla;i++)
	{
		tabla[i].compLex="-1";
	}
}

int esprimo(int n)
{
	int i;
	for(i=3;i*i<=n;i+=2)
		if (n%i==0)
			return 0;
	return 1;
}

int siguiente_primo(int n)
{
	if (n%2==0)
		n++;
	for (;!esprimo(n);n+=2);

	return n;
}

//en caso de que la tabla llegue al limite, duplicar el tama�o
void rehash()
{
	entrada *vieja;
	int i;
	vieja=tabla;
	tamTabla=siguiente_primo(2*tamTabla);
	initTabla();
	for (i=0;i<tamTabla/2;i++)
	{
		if(vieja[i].compLex!="-1")
			insertar(vieja[i]);
	}		
	free(vieja);
}

//insertar una entrada en la tabla
void insertar(entrada e)
{
	int pos;
	if (++elems>=tamTabla/2)
		rehash();
	pos=h(e.lexema,tamTabla);
	while (tabla[pos].compLex!="-1")
	{
		pos++;
		if (pos==tamTabla)
			pos=0;
	}
	tabla[pos]=e;

}
//busca una clave en la tabla, si no existe devuelve NULL, posicion en caso contrario
entrada* buscar(const char *clave)
{
	int pos;
	entrada *e;
	pos=h(clave,tamTabla);
	while(tabla[pos].compLex!="-1" && strcmp(tabla[pos].lexema,clave)!=0 )
	{
		pos++;
		if (pos==tamTabla)
			pos=0;
	}
	return &tabla[pos];
}

void insertTablaSimbolos(const char *s, char *cl)
{
	entrada e;
	sprintf(e.lexema,s);
	e.compLex=cl;
	insertar(e);
}

void initTablaSimbolos()
{
	insertTablaSimbolos(",","COMA");
	insertTablaSimbolos(".","PUNTO");
	insertTablaSimbolos(":","DOS_PUNTOS");
	insertTablaSimbolos("[","L_CORCHETE");
	insertTablaSimbolos("]","R_CORCHETE");
	insertTablaSimbolos("{","L_LLAVE");
	insertTablaSimbolos("}","R_LLAVE");
	insertTablaSimbolos("true","PR_TRUE");
	insertTablaSimbolos("false","PR_FALSE");
	insertTablaSimbolos("null","PR_NULL");
}

// Funcion para llegar al final de la linea
void linea_sgte()
{   char c;
	c=fgetc(archivo);
	while(c!='\n' && c!=EOF)
	{	c=fgetc(archivo);
	}
	ungetc(c,archivo);
}

void error(const char* mensaje)
{	printf("\nLin %d: Error Lexico. %s.",numLinea,mensaje);
	fprintf(archSalida,"\nLin %d: Error Lexico. %s.",numLinea,mensaje);
	linea_sgte();
}

void sigLex()
{
	int i=0, longid=0;
	char c=0;
	char linea[500];
	int acepto=0;
	int estado=0;
	char msg[41];
	entrada e;

	while((c=fgetc(archivo))!=EOF)
	{
		
		if (c==' ' || c=='\t')
			continue;	//eliminar espacios en blanco
		else if(c=='\n')
		{
			//incrementar el numero de linea
			numLinea++;
			continue;
		}
		else if (isalpha(c))
		{
			//puede ser true, false, null
			i=0;
			do{
				id[i]=tolower(c);
				i++;
				c=fgetc(archivo);
				if (i>=TAMLEX)
					break;
				
			}while(isalpha(c));
			id[i]='\0';
			if (c!=EOF)
				ungetc(c,archivo);
			else
				c=0;
			t.pe=buscar(id);
			t.compLex=t.pe->compLex;
			if (t.pe->compLex=="-1")
			{	error("Palabra clave incorrecta");
				break;
			}
			break;
		}
		else if (isdigit(c))
		{
				//es un numero
				i=0;
				estado=0;
				acepto=0;
				id[i]=c;
				
				while(!acepto)
				{
					switch(estado){
					case 0: //una secuencia netamente de digitos, puede ocurrir . o e
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=0;
						}
						else if(c=='.'){
							id[++i]=c;
							estado=1;
						}
						else if(tolower(c)=='e'){
							id[++i]=c;
							estado=3;
						}
						else{
							estado=6;
						}
						break;
					
					case 1://un punto, debe seguir un digito 
						c=fgetc(archivo);						
						if (isdigit(c))
						{
							id[++i]=c;
							estado=2;
						}
						else{
							if (c == '\n') 
							{
								ungetc (c, archivo);
								sprintf(msg,"No se esperaba \\n");
								estado=-1;
							}	
							else 
							{	sprintf(msg,"No se esperaba '%c'",c);
								estado=-1;
							}
						}
						break;
					case 2://la fraccion decimal, pueden seguir los digitos o e
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=2;
						}
						else if(tolower(c)=='e')
						{
							id[++i]=c;
							estado=3;
						}
						else
							estado=6;
						break;
					case 3://una e, puede seguir +, - o una secuencia de digitos
						c=fgetc(archivo);
						if (c=='+' || c=='-')
						{
							id[++i]=c;
							estado=4;
						}
						else if(isdigit(c))
						{
							id[++i]=c;
							estado=5;
						}
						else{
							if (c == '\n') 
							{
								ungetc (c, archivo);
								sprintf(msg,"No se esperaba \\n");
								estado=-1;
							}	
							else 
							{	sprintf(msg,"No se esperaba '%c'",c);
								estado=-1;
							}
						}
						break;
					case 4://necesariamente debe venir por lo menos un digito
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=5;
						}
						else{
							if (c == '\n') 
							{
								ungetc (c, archivo);
								sprintf(msg,"No se esperaba \\n");
								estado=-1;
							}	
							else 
							{	sprintf(msg,"No se esperaba '%c'",c);
								estado=-1;
							}
						}
						break;
					case 5://una secuencia de digitos correspondiente al exponente
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=5;
						}
						else{
							estado=6;
						}break;
					case 6://estado de aceptacion, devolver el caracter correspondiente a otro componente lexico
						if (c!=EOF)
							ungetc(c,archivo);
						else
							c=0;
						id[++i]='\0';
						acepto=1;
						t.pe=buscar(id);
						if (t.pe->compLex=="-1")
						{
							sprintf(e.lexema,id);
							e.compLex="LITERAL_NUM";
							insertar(e);
							t.pe=buscar(id);
						}
						t.compLex="LITERAL_NUM";
						break;
					case -1:
						if (c==EOF)
							error("No se esperaba el fin de archivo");
						else
							error(msg);
						acepto=1;
						t.compLex="-1";
						break;
						
					}
				}
			break;
		}
		else if (c==':')
		{
			t.compLex="DOS_PUNTOS";
			t.pe=buscar(":");
			break;
		}
		else if (c==',')
		{
			t.compLex="COMA";
			t.pe=buscar(",");
			break;
		}
		else if (c=='[')
		{
			t.compLex="L_CORCHETE";
			t.pe=buscar("[");
			break;
		}
		else if (c==']')
		{
			t.compLex="R_COCHETE";
			t.pe=buscar("]");
			break;
		}
		else if (c=='{')
		{
			t.compLex="L_LLAVE";
			t.pe=buscar("[");
			break;
		}
		else if (c=='}')
		{
			t.compLex="R_LLAVE";
			t.pe=buscar("]");
			break;
		}
		else if (c=='\"')
		{//un caracter o una cadena de caracteres
			i=0;
			id[i]=c;
			i++;
			do{
				c=fgetc(archivo);
				if (c=='\"')
				{
					id[i]=c;
					i++;
					break;
				}
				else if(c==EOF)
				{
					error("Se llego al fin de archivo sin finalizar un literal");
				}
				else if(c=='\n')
				{	ungetc(c,archivo);
					error("Se llego al fin de linea sin finalizar un literal"); 
					break;
				}
				else{
					id[i]=c;
					i++;
				}
			}while(isascii(c));
			id[i]='\0';
			if (c==EOF)
				c=0;
			if(id[i-1]!='\"' || i<2)
			{	t.compLex="-1";
				break;
			}
			t.pe=buscar(id);
			t.compLex=t.pe->compLex;
			if (t.pe->compLex=="-1")
			{
				sprintf(e.lexema,id);
				e.compLex="LITERAL_CADENA";
				insertar(e);
				t.pe=buscar(id);
				t.compLex=e.compLex;
			}
						
			break;
		}
	}
	if (c==EOF)
	{
		t.compLex="EOF";
		sprintf(e.lexema,"EOF");
		t.pe=&e;
	}
	
}

int main(int argc,char* args[])
{
	// inicializar analizador lexico
	char *complex;
	int linea=0;
	initTabla();
	initTablaSimbolos();
	archSalida=fopen("Salida.txt","w");
	if(argc > 1)
	{
		if (!(archivo=fopen(args[1],"rt")))
		{
			printf("Archivo no encontrado.\n");
			exit(1);
		}
		while (t.compLex!="EOF"){
			sigLex();
			int num_anterior;
			if (t.compLex!="-1")
			{	if(numLinea>linea)
				{	printf("\n%d: %s ",numLinea,t.compLex);
					fprintf(archSalida,"\n%d: %s ",numLinea,t.compLex);
					linea = numLinea;
				}
				else
				{	printf("%s ",t.compLex);
					fprintf(archSalida,"%s ",t.compLex);
				}
			}
		}
		fclose(archivo);
	}else{
		printf("Debe pasar como parametro el path al archivo fuente.\n");
		exit(1);
	}

	return 0;
}
