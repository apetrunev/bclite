.ds CH 
.nr PS 20
.nr VS 22
.nr HM 0.5i
.TL
.nf 
Интерпретируемый язык
для вычислений с плавающей точкой IEEE 754
.fi
.AU
\s16Научный руководитель к.ф.-м.н\s0 А. В. Ермоленко
\s16Исполнитель\s0 А. Л. Петрунёв
.AI
Сыктывкарский государственный университет

2012
.AB no
.AE
.
.bp 
.ce 
\fBЦель работы\fP

.IP \[bu] 2
Создать интерпретатор для языка программирования
.bp
.NH 1
Обзор
.PP
Под \fBинтерпретатором\fP обычно понимается компьютерная программа,
которая выполняет инструкции написанные на языке программирования.
.br

Перед тем как выполнить инструкции,
интерпретатор транслирует их в некоторое \fBпромежуточное представление\fP.
.bp

.PS
move down 1i
I:box width 2.3 height 1 "\fCИнтерпретатор\fP"
move
arrow right at 1/2 <I.se, I.ne> 0.1
R:box invisible width 1.5 "\fCРезультат\fP"
{
 B1: box wid 1.7 height 1 at (I.nw + (-1.2, 0.2)) "\fCИсходная\fP" "\fCпрограмма\fP"
 B2: box same at (I.sw + (-1.2, -0.2)) "\fCBходные\fP" "\fCданные\fP"
 arrow from B1.e to 2/3 <I.sw, I.nw>
 arrow from B2.e to 1/3 <I.sw, I.nw>
}
.PE

.bp 
.NH 1
Схема интерпретирования
.LP
Процесс интерпретирования можно разделить на несколько последовательных фаз:

.RS
.IP \[bu] 2
Лексический анализ
.IP \[bu]
Синтаксический анализ
.IP \[bu]
Семантический анализ
.IP \[bu]
Промежуточное представление
.IP \[bu]
Выполнение
.RE
.bp
.PS
down
box invisible width 2.0 height 0.3 "\fC\s16Поток символов\s0\fP"
arrow 0.15
box width 3 "\fCЛексический" "анализатор\fP"
line 0.1
box invisible width 2.0 height 0.3 "\fC\s16Поток токенов\s0\fP"
arrow 0.15
box width 3 "\fCСинтаксический" "анализатор\fP"
line 0.1
box invisible width 2.0 height 0.3 "\fC\s16Синтаксическое дерево\s0\fP"
arrow 0.15
box width 3 "\fCСемантический" "анализатор\fP"
line 0.1
box invisible width 2.0 height 0.3 "\fC\s16Синтаксическое дерево\s0\fP"
arrow 0.15;
box width 3 "\fCОбход дерева\fP"
arrow 0.15
box invisible width 2.0 height 0.3 "\fC\s16Результат\s0\fP"
move left 1.5i from 5th box .w
box width 2.5 "\fCТаблица" "символов\fP"
.PE
.bp 
.NH 1
Лексический анализ
.LP
Лексический анализатор группирует входные символы в \fBлексемы\fP.

.br
Для каждой лексемы создается соотвествующий \fBтокен\fP в форме:

.RS
\fC<имя_токена, зачение_атрибута>\fP
.RE

которая передается в последующую фазу, синтаксическому анализатору.

.bp
.LP
Например, лексический анализатор нашел число \fC256.0\fP.
.LP
синтаксический анализатор получит целое число \fCTOKEN_DOUBLE\fP,
со значением атрибута \fC256.0\fP.
.bp
.LP
Структура хранящая информацию о токене:
.RS
.nf
.CW

struct lex {
        token_t         token;
        union {
                char    *id;
                double  real;
                char    *string;
        };
};

.fi
.RE
.bp 


.LP
Предположим у нас есть выражение:
.ce
.nf

\fCy = 0.5*x + b\fP

.fi
Лексический анализатор видит данную последовательность как:
.ce
.nf

\fC(id, y)(=)(num, 0.5)(*)(id, x)(+)(id, b)\fP

.fi
где значение атрибута находится после запятой.

.bp 


.NH 1
Синтаксический анализ
.LP
Синтаксический анализатор делает следующее:
.RS

.IP \[bu] 2
Описывает язык

.IP \[bu]
Строит синтаксическое дерево, что представляет собой ту же самую программу,
только представленную в древовидной форме.
.RE
.bp
.LP
Язык описывается с помощью контекстно-свободных грамматик.
.nf
.CW
.ps 18
.vs 20

summ_expr \(-> mult_expr rest_summ

rest_summ \(-> + mult_expr rest_summ 
           | - mult_expr rest_summ | E

mult_expr \(-> term rest_mult

rest_mult \(-> * term rest_mult 
           | / term rest_mult | E

term \(-> digit | (summ_expr)

digit \(-> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

.ps
.vs
.fi
.bp 
.PS
circlerad = .35
movewid = .3
moveht = 1
move down 1i
right;
Eq: circle "\fC=\fP"
move down from Eq
{ move left; A: circle "\fCa\fP" }
{ move right; Plus: circle "\fC+\fP" }
move down from Plus
{ move left; B: circle "\fCb\fP" }
{ move right; Digit: circle "\fC10.0\fP" }
arrow from Eq to A chop
arrow from Eq to Plus chop
arrow from Plus to B chop
arrow from Plus to Digit chop
move left from A.w
move left .1
Str:box invisible width 2 "\fCa = b + 10.0\fP"
.PE
.bp
.LP
Структура хранящая информацию об узле дерева для различных операций:
.RS
.nf
.CW

struct ast_node_op {
	struct ast_node base;
	struct ast_node *left;
	struct ast_node *right;
	opcode_type_t opcode;
};

.fi
.RE
.
.bp
.
.NH 1
Семантический анализ
.IP \[bu] 2
Семантический анализатор использует синтаксичекое дерево и таблицу символов 
для проверки семантичекой согласованности с описанием языка.

.bp 
.NH 1
Oбход дерева

.IP \[bu] 2
Обход дерева начинается с \fBкорня\fP.

.IP \[bu]
Узлы дерева посещаются рекурсивно, в некотором порядке.
.bp
.LP
Обход дерева в глубину:
.PS
circlerad = .25
movewid = .39
moveht = 1
down
C1: circle "\fC+\fP"
move down from C1
{ move left;  C2: circle  "\fC*\fP" }
{ move right; C3: circle "\fC5\fP" } 
move down from C2
{ move left;  C4: circle "\fC2\fP" }
{ move right; C5: circle "\fC3\fP" }
arrow from C1 to C2 chop
arrow from C1 to C3 chop
arrow from C2 to C4 chop
arrow from C2 to C5 chop
spline -> from C1.n + (-.3, .1) to C2.w + (-.4, 0) \
then to C4.w  + (-.4, 0) \
then to C4.s  + (0, -.4) \
then to C4.se + (.4 , 0) \
then to C2.s  + (0, -.4) \
then to C5.w  + (-.4, 0) \
then to C5.s  + (0, -.4) \
then to C5.e  + (.4, 0)  \
then to C2.e  + (.4, 0)  \
then to C1.s  + (0, -.4) \
then to C3.w  + (-.4, 0) \
then to C3.s  + (0, -.4) \
then to C3.e  + (.4, 0)  \
then to C1.e  + (.4, 0)
.PE
