use Parse::RecDescent;
use Data::Dumper;
$::RD_AUTOACTION = q { [ @item ] };

#Manejo de archivos.
open (SALIDA, '>SimpleSemSource.txt');
open (ENTRADA, '<source.c3p');

#Mapa para convertir los operadores lógicos del lenguaje C3P a simplesem
%opLogicos=( "==" , "!=",
           "<>"   , "==",
           ">"    , "<=",
           "<="   , ">",
           ">="   , "<",
           "<"    , ">="
           );

#El FREE (D[1]) apunta a la posición 2 si no se declaran variables globales.
$numVarGlobal=2; 

#Lleva la cuenta de cuantas lineas de codigo se escribieron.
$numInstruc=1;

#Guarda un mapa para convertir nombres de variable a la dirección en que están.
%varToAddress=();

#Almacenará las instrucciones simplesem necesarias para llamar a una función.
$codigoFuncionAsignacion="";

#Utilizado para referenciar el valor devuelto por una función llamada.
$cantFuncionesAsignacion=0;

#Mapa de variables globales y locales {nombreFuncion}{nombreVariable}=direccion
%mapa=();

#Existe una palabra clave, numHilo, que permite referenciar el número de hilo para aplicaciones multihilo.
$mapa{GLOBAL}{"numHilo"}="RRRnumHilo";

#Hash de la forma clave: nombreFuncion  valor: lineaDeCodigo
%mapaFunciones=();

#Almacena las posiciones que deben guardarse para variables locales en cada función.
%cantVariablesFuncion=();

#Usado en regla programm para no buscar las variables paralelas si no hay parallel for
$hayParalelo=0;
$variableParalela;

#Guarda el espacio que ocupa la funcion en variables
$espacioFuncion=0; 

$grammar = q {

	programm :  declaration fun_decla main
			{
			#Cuando se matchea con declaration se separa cada variable con !!! para poder splitearlas acá.
			#En freeYdeclaraciones tenemos en la posición 0 el FREE, y en la posición 1 todas las declaraciones separadas por un espacio
				my @freeYdeclaraciones=split /!!!/,$item{declaration};
				if($freeYdeclaraciones[1] eq ''){
					$freeYdeclaraciones[0] = 2;
				}
				my $cadena="GLOBAL $freeYdeclaraciones[1]";
				
			#string to hash para recordar las variables. En $spliteado[0] tenemos la palabra GLOBAL
				my @spliteado=split / /,$cadena;
				my $nivel2=$spliteado[0]; #guarda la palabra GLOBAL para identificar el tipo de variable.
				
				
			#En la primera regla de declare_varlist se devuelve nombreVariable=PosdeMemoriaEnSimplesem con lo que podemos armar un mapa de cada variable global y la posición de memoria en la que está guardada, para que cuando se haga referencia a las variables en el código, sepamos por qué posición de memoria reemplazarla. Si la variable GLOBAL “a” se guarda en la posición de memoria 2, modificar la variable a en el código deberá traducirse en modificar la posición de memoria 2.
				my $n=1; #Paso el string con los nombres de variables y las direcciones donde están a un hash de 2 niveles

            #En $nivel1 y $nivel0 guardamos el nombre de la variable y la posición de memoria respectivamente
            #Iremos guardando entradas de la forma {GLOBAL}{nombreVar}=posicionMemoria
				while($n<@spliteado){
					(my $nivel1,my $nivel0)=split /=/,$spliteado[$n];
					$main::mapa{$nivel2}{$nivel1}=$nivel0; 
					$n++;
				}
					
			#imprimir el hash de variables GLOBAL
				while ((my $tipoVar,my $variables)=each %main::mapa ) {
				    print "\n$tipoVar --- \n";
				    while ( (my $nombreVar,my $dirMemoria) = each %$variables ) {
				        print "\t$nombreVar=>$dirMemoria \n";
				    }
					print "\n";
				}

                my $statement_block=$item{main};	
			#Luego de esto, $statement_block tiene todo el código con los nombres de variable.
			#Aquí puede hacerse un print para mostrarlo.
				
			#Cuando se encuentra un paralel_for se pone a 1 un flag para indicar que hay una parte en paralelo.
			#Lo que se hace a continuación es reemplazar las referencias a la variable paralela con la dirección de base de la variable más la palabra numHilo que indicará el offset de la variable con el ID de cada hilo.
				if($main::hayParalelo == 1){					
				#Reemplazamos los destinos, es decir las referencias a la variable paralela en el operando izquierdo.
					my $searchString="[?]{3}($main::variableParalela)"; #Busca ???(variableParalela)
					$statement_block =~ s/$searchString/"".&main::getVarAddress($1)." + numHilo"/ge;  
				#Reemplazamos los orígenes, es decir las referencias a la variable paralela en el operando derecho.
					$searchString="%%%($main::variableParalela)"; #Busca %%%(variableParalela)
					$statement_block =~ s/$searchString/"D[".&main::getVarAddress($1)."+numHilo]"/ge;
				}
			
			#En esta sección se modifica la manera en que se muestra el código simplesem para que se muestre de la manera correcta.
			#Ademas se limpia el código para quitar marcas agregadas que utiliza el parser.	
			#Primero se reemplazan los nombres de las variables por las posiciones de memoria, obteniéndolas con la función getVarAddres que busca en la tabla de variables Globales la dirección de memoria de una variable.
			#Reemplazamos los destinos, es decir todas las variables globales que están del lado izquierdo de la instrucción
				$statement_block =~ s/[?]{3}([a-zA-Z]\w*)/"".&main::getVarAddress($1)/ge;
			#Reemplazamos los orígenes, es decir todas las variables globales que están del lado derecho de la instrucción
				$statement_block =~ s/%%%([a-zA-Z]\w*)/"D[".&main::getVarAddress($1)."]"/ge;
			#Se modifican los arreglos: D[numero1][numero2] -> D[numero1+numero2]
				$statement_block =~ s/\[(\d+)\]\[(\d+)\]/"[".&main::sum($1,$2)."]"/ge;
			#Se modifican los arreglos con indice variable: D[numero1][D[numero2]] -> D[numero1+D[numero2]]
				$statement_block =~ s/\[(\d+)\]\[D\[(\d+)\]\]/"[$1 + D[$2]]"/ge;
			#Se modifican las expresiones de la forma numero1[numero2] -> numero1+numero2
				$statement_block =~ s/(\d+)\[(\d+)\]/&main::sum($1,$2)/ge;
			#Se modifican los arreglos con indice variable: numero1[D[numero2]] -> numero1+D[numero2]
				$statement_block =~ s/(\d+)\[D\[(\d+)\]\]/"$1 + D[$2]"/ge;
			#En el parallel for queda D[116][D[3+numHilo]]. Se modifica ][ por un +
				$statement_block =~ s/[\]][\[]/+/g;
			#Las funciones recursivas dejan un ### en los jumps cuando se hace un llamado recursivo. Borrar el ###
				$statement_block =~ s/###//g;
				
				
			#Sustituir las variables globales en las funciones. En el código simplesem, las variables globales de las
			#funciones tienen %%% o ??? de acuerdo a si están como destino, o como origen respectivamente.
			#La cantidad de variables locales de la función determina el tamaño del Registro de Activación,
			#por lo que el D[1] se asigna de la siguiente manera: set 1, D[1] + ### y cuando se conoce la cantidad
			#de variables locales (espacioFuncion) se remplaza ### por espacioFuncion.
				my $fun_decla=$item{fun_decla};
				$fun_decla =~ s/%%%([a-zA-Z]\w*)/"".&main::getVarAddress($1).""/ge; # Los del lado derecho
				$fun_decla =~ s/[?]{3}([a-zA-Z]\w*)/"".&main::getVarAddress($1)/ge; # Los del lado izquierdo
				$fun_decla =~ s/###/"$main::espacioFuncion"/ge; ## los del lado derecho (sin set)
				
			#Finalmente se imprime el resultado, que serán todas las instrucciones simplesem.
				"set 0, 0\n set 1, $freeYdeclaraciones[0]\n $statement_block"."halt"."$fun_decla";
			}

	declaration : declare ";" 
			{
			#Obtenemos la posición siguiente a la última variable global. Como el parser es de descenso recursivo, este código se ejecuta despues de haber entrado a todas las reglas de declare, por lo que, si dentro de estas reglas modificamos $numVarGlobal a medida que se agregan variables globales, cuando se ejecute este código tendremos en numVarGlobal la posición siguiente a la última variable global (FREE).
				my $nextFree=$main::numVarGlobal; # El valor de $nextFree va en el return de esta regla
				$main::numVarGlobal=2; # Resetear para cuando hay funciones con variables locales
				
			#Separaremos el nextFree y todas las declaraciones con !!! para splitearlo facilmente en la regla superior.
				"$nextFree!!!$item{declare}";
			}
			| ""

	declare : "var:" declare_varlist
			{ 
			#Devolvemos toda la lista de declaración de variables.
				$item{declare_varlist}; 
			} 

	declare_varlist : declare_var "," declare_varlist 
			{
			#En declare_var se coloca un espacio entre el nombre de la variable y el tamaño de la misma para poder hacer el split para @varYTam
				my @varYTam=split(' ',$item{declare_var});
				
			#$main::numVarGlobal tiene la dirección donde está el próximo espacio libre para variables globales.
			#Se guarda esa posición en el hash nombreVariable->posicionDeMemoria.
				$main::varToAddress{$varYTam[0]}=$main::numVarGlobal;
				
			#Se modifica numVarGlobal para que apunte al próximo espacio libre, y luego se devuelve
			#nombreVariable=PosicionDeMemoria, y separado por un espacio el resto de las declaraciones.
				my $devolver="$varYTam[0]=$main::numVarGlobal";
				$main::numVarGlobal+=$varYTam[1];
				"$devolver $item{declare_varlist}";
			}
			
			| declare_var
			{
			#En declare_var se coloca un espacio entre el nombre de la variable y el tamaño de la misma para poder hacer el split para @varYTam
				my @varYTam=split(' ',$item{declare_var});
				
			#$main::numVarGlobal tiene la dirección donde está el próximo espacio libre para variables globales.
			#Se guarda esa posición en el hash nombreVariable->posicionDeMemoria.	
				$main::varToAddress{$varYTam[0]}=$main::numVarGlobal;
				
			#Se modifica numVarGlobal para que apunte al próximo espacio libre, y luego se devuelve nombreVariable=2. Esto se debe a que la entrada es recursiva, y siempre que llegue al final de la recursion, que será cuando matchee con declare_var sin estar seguido por otro declare_varlist, el siguiente libre será la posición 2.
				$main::numVarGlobal+=$varYTam[1];
				"$varYTam[0]=2";
			}

	declare_var : declare_array 
			{
			#Si estamos declarando un array, devolvemos el nombre y el tamaño que ocupa. Con estos datos podemos ir armando la tabla de variables, con las posiciones de memoria que ocupa cada una.
			#Se coloca un espacio entre el id y el tamaño para hacer el split en la regla superior.
				"$item[1][1][1][1] $item{declare_array}[3][1]";
			}
			
			| iden
			{
			#Si se trata de un identificador de variable, devolvemos el nombre y el tamaño que ocupa, que será 1.
			#Se coloca un espacio entre el id y el tamaño para hacer el split en la regla superior.
				"$item[1][1][1] 1";
			}


	declare_array : iden "range" range_dims

	range_dims : range_value "," range_dims 
				| range_value

	range_value : int".."int
			{
			#El array multidimensional Item nos permitía ir entrando en los patrones matcheados. $Item[x][y][z] entra a la regla x actual, en ella entra a la regla y, y en esta última a la regla z, siendo el valor de retorno de cada regla lo que finalmente se obtiene.
			#$Item[3][2][1] devuelve el numero final del array. Ej: en range 2..6 devuelve el 6
			#$Item[1][2][1] devuelve el numero inicial del array. Ej: en range 2..6 devuelve el 2
			#Se devuelve el tamaño del array, haciendo rangoArrayFin - rangoArrayCzo + 1
				$item[3][2][1]-$item[1][2][1]+1;
			}
				
	fun_decla : iden "(" parameters ")" declaration statement_block
			{	
			#Cuando se matchea con declaration se separa cada variable con !!! para poder splitearlas acá.
			#En freeYdeclaraciones tenemos en la posición 0 el FREE, y en la posición 1 todas las declaraciones separadas por un espacio
				my @freeYdeclaraciones=split /!!!/,$item{declaration};
								
			#Separamos los parámetros para saber cuántos hay.
				my @parametros = split/ /,$item{parameters};
				my $cantParametros = @parametros;

			#Calculamos el espacio que ocupará la función, para saber dónde quedará apuntando el siguiente libre.
				$main::cantVariablesFuncion{$item{iden}[1][1]}=$freeYdeclaraciones[0] + @parametros;
				$main::espacioFuncion=$freeYdeclaraciones[0] + @parametros;
			
			#Corregimos la posición de las variables, ya que si hay parámetros, las variables irán después de estos.
				my $variablesYParametros = $freeYdeclaraciones[1];
				my $numVarAux = $main::cantVariablesFuncion{$item{iden}[1][1]} - 1;
				$variablesYParametros =~ s/\d+/$numVarAux++/ge;
				
			#Agregamos al string variablesYParametros, los parámetros, ya que hasta ahora solo tiene las variables.
			#El primero parámetro irá en la posición 2, y en las sucesivas los otros parámetros. Las variables empiezan en la posición siguiente a la del último parámetro.
				my $parameterPos = 2;
				while($parameterPos-2 < @parametros){
					$variablesYParametros = $variablesYParametros." $parametros[$parameterPos-2]=$parameterPos";
					$parameterPos++;
				}
				
			#En $cadena, obtenemos el nombre de la función, y separadas por espacio entradas de la forma "nombreVariable=offset_en_registro_de_activación".
				my $cadena="$item{iden}[1][1] $variablesYParametros";
				
			#En @spliteado tenemos el nombre de la funcion en la posición 0 y en las siguientes las variables.
				my @spliteado=split / /,$cadena;
				
			#En $nivel2 guardamos el nombre de la función.
				my $nivel2=$spliteado[0];
			
			#Luego, guardamos el nombre ($nivel1) y la posición relativa ($nivel0) en el mapa de variables con dos llaves (nombreFuncion y nombreVariable), tieniendo en cuenta que los parámetros de la función están antes que las variables locales.
				my $n=1;
				while($n<@spliteado){
					(my $nivel1,my $nivel0)=split /=/,$spliteado[$n];
					$nivel0 = $nivel0;
					$main::mapa{$nivel2}{$nivel1}=$nivel0;
					$n++;
				}
						
				my $statement_block=$item{statement_block};
			#Reemplazamos los destinos, es decir todas las variables globales que están del lado izquierdo de la instrucción
				$statement_block =~ s/[?]{3}([a-zA-Z]\w*)/"".&main::getVarAddress($1,$item{iden}[1][1],1)/ge;
			#Reemplazamos los orígenes, es decir todas las variables globales que están del lado derecho de la instrucción
				$statement_block =~ s/%%%([a-zA-Z]\w*)/"D[".&main::getVarAddress($1,$item{iden}[1][1],0)."]"/ge;
			#Se modifican los arreglos: D[numero1][numero2] -> D[numero1+numero2]
				$statement_block =~ s/\[(\d+)\]\[(\d+)\]/"[".&main::sum($1,$2)."]"/ge;
			#Se modifican los arreglos con indice variable: D[numero1][D[numero2]] -> D[numero1+D[numero2]]
				$statement_block =~ s/\[(\d+)\]\[D\[(\d+)\]\]/"[$1 + D[$2]]"/ge;
			#Se modifican las expresiones de la forma numero1[numero2] -> numero1+numero2
				$statement_block =~ s/(\d+)\[(\d+)\]/&main::sum($1,$2)/ge;
			#Se modifican los arreglos con indice variable: numero1[D[numero2]] -> numero1+D[numero2]
				$statement_block =~ s/(\d+)\[D\[(\d+)\]\]/"$1 + D[$2]"/ge;
				
			#En las funciones las variables se referencian a partir de D[0], es decir si T esta en la direccion 3, T se referencia como D[0]+2 en lado izquierdo y D[D[0]+2] en lado derecho	
				$statement_block =~ s/set[\s]([2-9]+)/"set D[0]+ $1"/ge; #Lado izquierdo
				$statement_block =~ s/D\[([2-9]+)\]/"D[D[0]+$1]"/ge; #Lado derecho
			
			#Devolvemos las instrucciones de la función, y las instrucciones que necesitamos para volver de la función.
				"\ndef_Funcion $item[1][1][1] $statement_block set 1,D[0] \n set 0, D[D[0]+1] \n jump D[D[1]] \n ";
			}
			
			| ""
			{
				"";
			}

	statement_block : "{" statements "}"
			{
			#Devolvemos directamente los statements
				"$item[2]";
			}
	
	parameters : parameter "," parameters
			{
			#Devolvemos el primer parámetro, seguido de los otros parámetros. Al entrar recursivamente, se devuelven todos los parámetros separados por dos barras invertidas.
				"$item[1] $item{parameters}";
			}
			
			| parameter
			{
			#Al llegar al último elemento (un único parámetro) devolvemos el nombre de ese parámetro.
				"$item[1]";
			}

	parameter : iden
			{
				$item[1][1][1];
			}
	
			| ""
			{
				"";
			}

	statements : statement statements
			{
			#Devolvemos el statement actual, seguido de los otros statements. Cada statement es una instrucción simplesem con algunas marcas para el parseo posterior, como los nombres de variables precedidas de %%%, para luego ser reemplazadas por las posiciones de memoria donde se encuentran.
				"$item[1] $item{statements}";
			}
			
			| statement
			{
			#Devolvemos el statement matcheado
				$item{statement};
			}
			
			| ""
			{
				"";
			}

	statement : assign ";"
			{
			#Devolvemos lo que devuelve la regla assign.
				$item{assign};
			}
			
			| "read" "(" iden ")" ";" 
			{
				my $aux;
			#Con este if-else ponemos en $aux el nombre de la variable o del elemento del arreglo.
				if ($item{iden}[1][0] eq "single_iden"){
					$aux="???$item{iden}[1][1]";
				}
				else{
					$aux="???$item{iden}[1][1][1]"."[$item{iden}[1][2][2]]";
				}
				
			#Finalmente imprimimos la instrucción para tomar una entrada de usuario, y guardarla en una variable, que luego será reemplazada por una posición de memoria.
				"set $aux, read\n";
			}
			
			| "write" "(" expr ")" ";" 
			{
			#La variable $codigoFuncionAsignacion contiene las instrucciones necesarias para llamar a una función, que en simplesem se traducen siempre al mismo conjunto de instrucciones.
				my $codigoFuncion="";
				$codigoFuncion=$main::codigoFuncionAsignacion;
				$main::codigoFuncionAsignacion=""; #limpiamos para la siguiente asignacion
				$main::cantFuncionesAsignacion=0;  #limpiamos para la siguiente asignacion
			#Devolvemos el código simplesem que genera la salida en pantalla.
				$codigoFuncion."set write, $item{expr}\n";
			}
			
			| iterate 
			{
			#Devolvemos lo que devuelve la regla iterate.
				$item{iterate};
			}
			
			| question
			{
			#Devolvemos lo que devuelve la regla question.
				$item{question};
			}
			
			| "return" expr ";"
			{
			#Devolvemos el código simplesem para escribir en la posición anterior al inicio del registro de activación, donde se coloca el valor de rotorno.
				"set D[0]-1, $item{expr}\n";
			}
			
			| "wait" "(" iden ")" ";"
			{
			#Con este if-else ponemos en $aux la instruccion set con el nombre de la variable o del elemento del arreglo.
				my $aux;
				if ($item[3][1][0] eq "single_iden"){
					$aux="???$item[3][1][1]";
				}
				else{
					$aux="???$item[3][1][1][1]+"."$item[3][1][2][2]";
				}
				"wait $aux \n";
			}
			
			| "notify" "(" iden ")" ";"
			{
			#Con este if-else ponemos en $aux la instruccion set con el nombre de la variable o del elemento del arreglo.
				my $aux;
				if ($item[3][1][0] eq "single_iden"){
					$aux="$item[3][1][1]";
				}
				else{
					$aux="$item[3][1][1][1]+"."$item[3][1][2][2]";
				}
				"set ???$aux,  D[???$aux]+1 \n";
			}
			
			| call ";" 
			{
				my $ip4MasParametros=4;
				my $parametros="";
					
			#Tomamos todos los argumentos del call en un array, y los agregamos en el llamado de la función.
				my @argumentos = split / /, $item[1][3];
				my $n=0;
				while($n<@argumentos){
					$parametros .= "set D[1]+".(2+$n).",$argumentos[$n]\n";
					$ip4MasParametros++;
					$n++;
				}
					
				$main::codigoFuncionAsignacion=$main::codigoFuncionAsignacion."set 1, D[1]+1 (llamar func $item[1][1][1][1])\n set D[1], ip + $ip4MasParametros \n set D[1]+1, D[0] \n $parametros set 0,D[1] \n set 1,D[1] + ###$main::cantVariablesFuncion{$item[1][1][1][1]} \n jump $item[1][1][1][1]  \n";
				$main::cantFuncionesAsignacion++;
				
			#La variable $codigoFuncionAsignacion contiene las instrucciones necesarias para llamar a una función, que en simplesem se traducen siempre al mismo conjunto de instrucciones.
				my $codigoFuncion="";
				$codigoFuncion=$main::codigoFuncionAsignacion;
				$main::codigoFuncionAsignacion=""; #limpiamos para la siguiente asignacion
				$main::cantFuncionesAsignacion=0;  #limpiamos para la siguiente asignacion
			#Devolvemos el código simplesem que genera la salida en pantalla.
				$codigoFuncion	
			}
			
			| thread_definition ";"
			
			| parallel_statement
			{
			#Devolvemos lo que devuelve la regla parallel_statement.
				$item{parallel_statement};
			}
			
			| comment
			{
				$item{comment};
			}

	thread_definition : "define" int "threads"

	main : "program" statement_block
			{
			#Devolvemos directamente el bloque de instrucciones.
				"$item{statement_block}";
			}
			
			| ""
			
	assign : iden "=" expr 
			{
			#La variable $codigoFuncionAsignacion contiene las instrucciones necesarias para llamar a una función, que en simplesem se traducen siempre al mismo conjunto de instrucciones.
				my $codigoFuncion="";	
				$codigoFuncion=$main::codigoFuncionAsignacion;
				$main::codigoFuncionAsignacion=""; #limpiamos para la siguiente asignacion
				$main::cantFuncionesAsignacion=0;  #limpiamos para la siguiente asignacion
			
			#Con este if-else ponemos en $aux la instruccion set con el nombre de la variable o del elemento del arreglo.
				my $aux;
				if ($item[1][1][0] eq "single_iden"){
					$aux="set ???$item[1][1][1]";
				}
				else{
					$aux="set ???$item[1][1][1][1]"."[$item[1][1][2][2]]";
				}
				
			#Devolvemos las instrucciones necesarias para llamar a una función (por si la expresión contiene una llamada a función), más la instrucción de asignación. Los signos ??? de $aux se usan para reemplazar luego los nombres de variables por las posiciones de memoria a las que son asignadas.
				$codigoFuncion."$aux, $item{expr}	(asignacion)\n";
			}
			
			| var_inc 
			{
				$item[1];
			}
			
			| var_dec
			{
				$item[1];
			}
			


	var_inc : iden "++" 
			{
			#Con este if-else ponemos en $aux la instruccion set con el nombre de la variable o del elemento del arreglo.
				my $aux;
				if ($item[1][1][0] eq "single_iden"){
					$aux="set ???$item[1][1][1],  %%%$item[1][1][1]+1	(asignacion-incremento)\n";
				}
				else{
					$aux="set ???$item[1][1][1][1]"."[$item[1][1][2][2]]".",  %%%$item[1][1][1][1]"."[$item[1][1][2][2]]+1	(asignacion-incremento)\n";
				}
			
			#Devolvemos lo que hay en $aux.	
				$aux;
			}

	var_dec : iden "--"
			{
			#Con este if-else ponemos en $aux la instruccion set con el nombre de la variable o del elemento del arreglo.
				my $aux;
				if ($item[1][1][0] eq "single_iden"){
					$aux="set ???$item[1][1][1],  %%%$item[1][1][1]-1	(asignacion-decremento)\n";
				}
				else{
					$aux="set ???$item[1][1][1][1]"."[$item[1][1][2][2]]".",  %%%$item[1][1][1][1]"."[$item[1][1][2][2]]-1	(asignacion-decremento)\n";
				}
				
			#Devolvemos lo que hay en $aux.	
				$aux;
			}
	 
	iterate : while_iteration 
			{
			#Devolvemos lo que devuelve la regla while_iteration.
				$item{while_iteration};
			}
			
			| for_iteration
			{
			#Devolvemos lo que devuelve la regla for_iteration.
				$item{for_iteration};
			}

	while_iteration : "while" "(" bool_expr ")" statement_block
			{
			#Se cuentan la cantidad de lineas que hay en el statement block para hacer los jump del while. 
			#El numero se pone precedido de +++, por ejemplo +++7, para encontrarlo mas facil en el texto.
			#Esto se hace para que una vez que se sepa en qué linea está, se hace un jump a esa línea mas 7.
				my $cuentaSaltoWhile=0;
				$cuentaSaltoWhile++ while ($item{statement_block}=~/[\n]{1}/g);
				$cuentaSaltoWhile+=1;
			
			#Devolvemos todo el bloque simplesem que ejecuta el while.
				"jumpt +++".&main::sum($cuentaSaltoWhile,1).", $item{bool_expr} $item{statement_block} jump ---$cuentaSaltoWhile \n";
			}

	for_iteration : "for" for_definition statement_block
			{
			#$cuentaSaltoFor cuenta la cantidad de líneas que hay en el statement block para hacer los jump del for.
			#El número se pone por ejemplo +++7. El 7 indica un valor relativo, y el +++ se utiliza para detectar dónde está el salto, y modificarlo por la dirección absoluta.
				my $cuentaSaltoFor=0;
				$cuentaSaltoFor++ while ($item{statement_block}=~/[\n]{1}/g);
				$cuentaSaltoFor+=2; #contar la asignacion al final del for y el jump para volver
				
			#Devolvemos un bloque de código que será:
			#set ???variable_for, 0 (inicialización de la variable del for)
			#jumpt +++linea_siguiente_al_jump, %%%variable_for  [condicion] (Evaluación de la condición para saltar hacia adelante y salir del for)
			#...  
			#...   (Operaciones que se realizan dentro del for)
			#...
			#set ???variable_for,  %%%variable_for+1	(Incremento de la variable de control)
			#jump ---linea_del_jumpt (Salto a la posición de memoria donde está la evaluación de la condición)
			
				"$item{for_definition}[2] jumpt +++".&main::sum($cuentaSaltoFor,1).", $item{for_definition}[4] $item{statement_block}"."$item{for_definition}[6] jump ---$cuentaSaltoFor\n";
			}

	for_definition : "(" assign ";" bool_expr ";" assign ")"

	question : "if" "(" bool_expr ")" statement_block "else" statement_block
			{
			#Se cuenta la cantidad de líneas que hay en el statement block para hacer los jump del if y del else.
			#El número se pone por ejemplo +++7. El 7 indica un valor relativo, y el +++ se utiliza para detectar dónde está el salto, y modificarlo por la dirección absoluta.
				my $cuentaSaltoIf=0;
				my $cuentaSaltoElse=0;
				$cuentaSaltoIf++ while ($item[5]=~/[\n]{1}/g);
				$cuentaSaltoElse++ while ($item[7]=~/[\n]{1}/g);
			#Corrección del offset de los saltos del if y del else.
				$cuentaSaltoIf+=2;
				$cuentaSaltoElse+=1;
			
			#Devolvemos el código simplesem generado.
				"jumpt +++$cuentaSaltoIf, $item{bool_expr} $item[5]"."jump +++$cuentaSaltoElse \n $item[7]";	
			}

			| "if" "(" bool_expr ")" statement_block
			{
			#Se cuenta la cantidad de líneas que hay en el statement block para hacer los jump del if y del else.
			#El número se pone por ejemplo +++7. El 7 indica un valor relativo, y el +++ se utiliza para detectar dónde está el salto, y modificarlo por la dirección absoluta.
				my $cuentaSaltoIf=0;
				$cuentaSaltoIf++ while ($item[5]=~/[\n]{1}/g);
			#Corrección del offset de los saltos del if y del else.
				$cuentaSaltoIf+=1;
				
			#Devolvemos el código simplesem generado.
				"jumpt +++$cuentaSaltoIf, $item{bool_expr} $item{statement_block}";
			}

	thread_definition : "define" int "threads"

	parallel_statement : parallel_for 
			{
			#Devolvemos lo que devuelve la regla parallel_for
				$item{parallel_for};
			}
			
			| "barrier" ";"
			{
				"barrier\n";
			}

	parallel_for : parallel_for_beginning statement_block parallel_for_end
			{
			#Avisamos al programa que hay un bloque en paralelo.
				$main::hayParalelo =1;
			#Extraemos la variable paraela y sus rangos (comienzo y final)
			#El nombre de la variable paralela se extrae del primer assign en for definition, ya que tiene la siguiente forma:
			#par_for 4 (n = 0; n < 10; n = n+1);
				$main::variableParalela=$item[1][3][2];
				my $valorInicial=0;
				my $valorFinal=0;
			
			#Extraemos el nombre de la variable y el valor inicial
				$main::variableParalela =~ s/.*[?]{3}([a-zA-Z]\w*),\s([\d+]).*[\n]/"$1 $2"/ge;
				($main::variableParalela,$valorInicial)=split / /,$main::variableParalela;
				
			#De la bool_exp del for extraemos el valor final de la variable paralela.
				$valorFinal=$item[1][3][4];
				$valorFinal =~ s/.*\s([\d]+)\s.*[\n]/"$1"/ge;
				
			#Teniendo el valor final y el valor inicial de la variable paralela, debemos ver cuánto abarca, para saber los límites de los for en cada hilo haciendo "Rango de variableParalela / Nº de hilos".
			#Esto se hace ya que todas las variables locales a cada hilo en un parallel for, estarán en posiciones de memoria contiguas, por lo que se referenciarán como: baseDeVariable + numHilo, siendo el número de hilo el offset que le da acceso a la variable local de un hilo particular.
				my $multiplicador = int(($valorFinal-$valorInicial+1)/$item[1][2][1]);
				my $totalVarParalela = $multiplicador * $item[1][2][1];
				
			#$asignaInicial tendrá las asignaciones que se hacen a la variable de control del parallel for. En principio, $asignaInicial solo tendrá la referencia a la base, y luego le sumamos "numHilo*$multiplicador" para tener la referencia a la variable local de cada hilo.
				my $asignaInicial=$item[1][3][2];
				$asignaInicial =~ s/([\d]+)/"$1 + numHilo*$multiplicador"/ge;
				
			#La expresión booleana del for es distinta para cada hilo. Suponiendo que se quiere abarcar i de 0 a 7, en 4 hilos, el hilo 0 irá de i=0 hasta i < 2, el hilo 1 irá de i=2 hasta i < 4, el hilo 2 irá de i=4 hasta i < 6, y el hilo 3 irá de i=6 hasta i < 8.
				my $exprBoolFor=$item[1][3][4];
				$exprBoolFor =~ s/([\d]+)/"$multiplicador*(numHilo+1) + $valorInicial - 1"/ge;
			#La siguiente línea corrige un error, ya que si teníamos que i iba de 0 a 7 por ejemplo, pero lo expresábamos como i < 8, por la manera en que generamos la condición del parallel for, tenemos que usar el operador > en vez del >=
				$exprBoolFor =~ s/>=/>/;
				
			#Debemos poner la ultima linea del for definition despues del statement block
			#Se cuenta la cantidad de líneas que hay en el statement block para hacer los jump del for.
			#El número se pone por ejemplo +++7. El 7 indica un valor relativo, y el +++ se utiliza para detectar dónde está el salto, y modificarlo por la dirección absoluta.
				my $cuentaSaltoFor=0;
				$cuentaSaltoFor++ while ($item{statement_block}=~/[\n]{1}/g);
			
			#Corregimos el offset del salto, agregando 2 por la asignación final del for, y por el jump para volver a la condición.
				$cuentaSaltoFor+=2;
		
			#Devolvemos todas las sentencias del parallel for.
				"processors $item[1][2][1] \nparallel_for $totalVarParalela, $valorInicial  \n $asignaInicial jumpt +++".&main::sum($cuentaSaltoFor,1).", $exprBoolFor $item{statement_block}"."$item[1][3][6] jump ---$cuentaSaltoFor\n end_par_for\n";
			}

	parallel_for_beginning : "par_for" numbers for_definition
	 
	parallel_for_end : "end_par_for" ";"

	bool_expr :  expr comp expr
			{
			#Devolvemos la expresion, los operadores lógicos y la otra expresión.
			#Esto queda algo de la forma <expresion1> [op_logico] <expresion2> que será evaluado por el intérprete.
				"$item[1]  $main::opLogicos{$item[2][1]} $item[3] (bool_exp)\n";
			}


	comp : "==" | "<>" | ">=" | "<=" | ">" | "<"

	call : iden "(" args ")"
	
	args : expr "," args
			{
			#Devolvemos todos los argumentos separados por un espacio asi pueden ser spliteados en la regla superior (expr).
				"$item[1] $item[3]";
			}
	
			| expr
			{
				$item[1];
			}
			
			| ""
			{
				"";
			}

	exprs : expr exprs 
			{
			#Devolvemos la expresión actual, seguida de las demás expresiones.
				"$item[1] $item[2]";
			}
			
			| expr 
			{
				$item[1];
			}
			
			| ""
			{
				"";
			}

	expr : factor operator expr 
			{
			#En $aux guardamos lo que necesitamos de la regla "factor" de la expresión.
				my $aux=111; 
				
			#En el caso de que el factor sea un int, tomamos en $aux el numero.
				if($item[1][1][0] eq "int"){
					#$aux=$item[1][1][2][1];           #signo $item[1][1][1][1]
					$aux="$item[1][1][1][1]$item[1][1][2][1]";
				}
			
			#En el caso de que el factor sea un iden, debemos sustituir los nombres de las variables por %%%nombreVar para luego parsearlas fácilmente y reemplazar los nombres de variables por la posición de memoria que ocupan.
			#Ademas, debemos ver si las variables son single (if), o son arrays (else).
				if($item[1][1][0] eq "iden"){
					if ($item[1][1][1][0] eq "single_iden"){
						$aux="%%%$item[1][1][1][1]";
					}
					else{
						$aux="%%%$item[1][1][1][1][1]"."[$item[1][1][1][2][2]]";
					}
				}
			
			#En el caso de que el factor sea un call, tomamos el ip +4 si la función no tiene parámetros, como dirección de retorno, y si tiene parámetros, sumamos 1 por cada parámetro que se pasa a la función.
				if($item[1][1][0] eq "call"){
					my $ip4MasParametros=4;
					my $parametros="";
					
			#Tomamos todos los argumentos del call en un array, y los agregamos en el llamado de la función.
					my @argumentos = split / /, $item[1][1][3];
					my $n=0;
					while($n<@argumentos){
						$parametros .= "set D[1]+".(2+$n).",$argumentos[$n]\n";
						$ip4MasParametros++;
						$n++;
					}
					
					$main::codigoFuncionAsignacion=$main::codigoFuncionAsignacion." set 1, D[1]+1 (llamar func $item[1][1][1][1][1]) \n set D[1], ip + $ip4MasParametros \n set D[1]+1, D[0] \n $parametros set 0,D[1] \n set 1,D[1] + $main::cantVariablesFuncion{$item[1][1][1][1][1]} \n jump $item[1][1][1][1][1]  \n "; 
					$main::cantFuncionesAsignacion++;
					$aux="D[D[1]-$main::cantFuncionesAsignacion]"; 
				}
				"$aux $item{operator}[1] $item{expr}";
			}
			
			| factor
		 	{
		 	#En $aux guardamos lo que necesitamos de la regla "factor" de la expresión.
				my $aux=111;
				
			#En el caso de que el factor sea un int, tomamos en $aux el numero.
				if($item[1][1][0] eq "int") {
					#$aux=$item[1][1][2][1];           #signo $item[1][1][1][1]
					$aux="$item[1][1][1][1]$item[1][1][2][1]";
				}
				
			#En el caso de que el factor sea un iden, debemos sustituir los nombre de las variables por %%%nombreVar para luego parsearlas fácilmente y reemplazar los nombres de variables por la posición de memoria que ocupan.
			#Ademas, debemos ver si las variables son single (if), o son arrays (else).
				if($item[1][1][0] eq "iden"){
					if ($item[1][1][1][0] eq "single_iden"){
						$aux="%%%$item[1][1][1][1]";           #signo $item[1][1][1][1]
					}
					else{
						$aux="%%%$item[1][1][1][1][1]"."[$item[1][1][1][2][2]]";
					}
				}
				
			#En el caso de que el factor sea un call, tomamos el ip +4 si la función no tiene parámetros, como dirección de retorno, y si tiene parámetros, sumamos 1 por cada parámetro que se pasa a la función.
				if($item[1][1][0] eq "call"){
					my $ip4MasParametros=4;
					my $parametros="";
					
			#Tomamos todos los argumentos del call en un array, y los agregamos en el llamado de la función.
					my @argumentos = split / /, $item[1][1][3];
					my $n=0;
					while($n<@argumentos){
						$parametros .= "set D[1]+".(2+$n).",$argumentos[$n]\n";
						$ip4MasParametros++;
						$n++;
					}
					
					$main::codigoFuncionAsignacion=$main::codigoFuncionAsignacion."set 1, D[1]+1 (llamar func $item[1][1][1][1][1])\n set D[1], ip + $ip4MasParametros \n set D[1]+1, D[0] \n $parametros set 0,D[1] \n set 1,D[1] + ###$main::cantVariablesFuncion{$item[1][1][1][1][1]} \n jump $item[1][1][1][1][1]  \n";
					$main::cantFuncionesAsignacion++;
					$aux="D[D[1]-$main::cantFuncionesAsignacion]";  
				}
				$aux;
		 	}

	factor : call | iden | int

	int : sign digits

	digits : /[0-9]+/

	sign : "-" | ""

	iden : array_iden | single_iden

	single_iden : /[a-zA-Z]\\w*/

	array_iden : single_iden array_pos

	array_pos : "["array_dim_pos"]"

	array_dim_pos : expr
					{
						$item[1];
					}

	letter_or_digits : /\\w*/ 

	digit : "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

	numbers: /\d+/
	
	operator : "+" | "-" | "/" | "*"
	
	letter : /[a-zA-Z]/
	
	comment : /\/\/.*/ 
			{
				"";
			}

};

#Creamos el parser con la gramática definida por las reglas de producción de $grammar.
$parser = Parse::RecDescent->new($grammar) or die "Gramática errónea\n";

#Leemos el archivo que contiene el código fuente.
while (<ENTRADA>){
	$codigo = $codigo.$_;
}

$salida=$parser->programm($codigo);
$salida=$salida."\n";
$var = Dumper($salida); 

#Configuramos el string $salida para que se imprima en un archivo.
print $salida;
print "\n-----\n\n";
$numLinea=0;
$salida=~ s/"\n \n"/"\n"/ge; #agregar numeros de lineas
$salida=~ s/(.*)[\n]{1}/$main::numLinea++." $1\n"/ge; #agregar numeros de lineas
$salida=~ s/([\d]+)\sdef_Funcion\s([\w]+)/agregarAlMapa($2,$1)/ge; #vemos en que lineas se definen las funciones
$salida=~ s/jump\s([\w]+)[^\[]/"jump $mapaFunciones{$1}"/ge; #jump de las funciones. Ell jump ReturnAddress es D[D[1]] asi que para que no lo saque se fija que no haya corchetes con esta regex  [^\[].
$salida=~ s/([\d]+)\s*jump([t]?)\s([+-]{3})([\d]+)/"$1  jump$2 ".sumRestaConSimbolo($1,$4,$3)/ge; #jump de if,if-else,for,while. identificados con jump +++[\d]

print $salida;
print "\n-----\n\n";

#imprimir SALIDA al archivo que va al interprete
#. Sacar numeros de linea y mapeo de variables

$salida=~ s/[\d]+[\s]*(.*[\n]{1})/$1/g; # sacar los numeros de linea en las que tienen instrucciones
$salida=~ s/D\[RRRnumHilo\]/numHilo/g; # Reemplazar las referencias a numHilo que estan como D[RRRnumHilo] a numHilo.
$salida=~ s/[\n]{1}[\d]+(.*[\n]{1})/$1/g; #sacar
$salida=~ s/\([\w\s-]+\)//g;


print $salida;
print SALIDA $salida;

close (SALIDA); 

#------Sub rutinas---------

#Dado un nombre de una variable y (opcional) el de una funcion, devuelva la direccion.
sub getVarAddress {
  my ($varName,$subName,$ladoIzq) = @_;
  
  #primero vemos si es variable local
  my $dire=$main::mapa{$subName}{$varName};
  if ($dire eq "")
	{
	if ($subName ne "")
		{
		$dire="%%%$varName";
		if($ladoIzq==1)
			{
			$dire="???$varName";
			}
		else
			{
			$dire="%%%$varName";
			}
		}
	# caso contrario, la estamos llamando desde global
	else {
		{
		$dire=$main::mapa{'GLOBAL'}{$varName};
		}
	}
	}
  $dire;
}

#Recibe el nombre de la funcion y la linea en la que esta. Los agrega al mapa
sub agregarAlMapa {
  my ($funcion,$linea) = @_;
  $mapaFunciones{$funcion}=$linea;
  "$linea ";
}

#Devuelve la suma de dos elementos
sub sum {
  my ($num1,$num2) = @_;
  $num1+$num2;
}




#Recibe dos numeros y un simbolo +++ o --- si es +++ hace suma, sino resta
sub sumRestaConSimbolo {
  my ($num1,$num2,$simbolo) = @_;
  my $ret;
  if ($simbolo eq "+++"){
	$ret=$num1+$num2;
  }
  else{
	$ret=$num1-$num2;
  }
  $ret;
}


#CODIGO TESTING

#Parallel_for example. Suma los numeros del 2 al 9 y del 1 al 8, resultado = 80.
=cut
var: a range 1..8, b range 1..8, suma, i range 1..4, j;

program{
	
    for(j=0;j <= 7; j++){
        a[j] = 1 + j;
        b[j] = 2 + j;
    }

    par_for 4 (i = 0;i <= 7;i++){
        suma = suma + a[i];
        barrier;
		suma = suma + b[i];
	}
    end_par_for;
    write(suma);
}
=cut

#Ejemplo Numeros Primos. muestra los numeros primos del 1 hasta un numero ingresado (limite = 199)
=cut
var: esPrimo range 1..200,j, i, max;
program{
		
	read(max);
	max--;
	for(j=0;j < max; j++){
		esPrimo[j] = 1;
	}

	for(j=2;j < max; j++){
		if(esPrimo[j]==1){
			for(i=2*j;i < max; i=i+j){
				esPrimo[i]=0;
			}
		}
	}

	for(j=0;j < max; j++){
		if(esPrimo[j]==1){
			write(j);
		}
	}
}
=cut

#Ejemplo del cálculo del factorial de un número.
=cut
var: n;
fact()
var: loc;
{
	if (n > 1){
		loc = n;
		n = n - 1;
		loc = loc * fact();
		return loc;
	}
	else {
		return 1;
	}
}

program{
	read(n);
	if (n >= 0){
		write(fact());
	}
	else{
		write(0);
	}
}
=cut

#Ejemplo que encuentra el minimo comun divisor entre dos números.
=cut
var: numMay, numMin, resto;
program {
	resto = 1;
	read(numMay);
	read(numMin);
	while(resto <> 0){
		while(numMay >= 0){
			numMay = numMay - numMin;
		}
		numMay = numMay + numMin;
		resto = numMay;
		numMay = numMin;
		numMin = resto;
	}
	write(numMay);
}
=cut

#Ejemplo en el que se ve una función que recibe varios parámetros.
=cut
var: n, j;
suma(num1, num2, num3, num4)
var: loc;
{
	loc = -4;
	num1 = num1 + num2 + num3 + num4 + loc;
	return num1;
}

program{
	n = 5;
	j = 6;
	write(suma(n, 4, j, 3));
}
=cut
