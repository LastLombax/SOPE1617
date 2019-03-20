# SOPE1617

## Description

Queue management for saunas, with a multi-threaded system, using two different programs

## Usage

### Generator Program

_Invocation:_ ./generator <number of requests> <max. usage>

The program prints all requests since its creation. It also keeps the rejected requests and prints them in the end.
In the end, some statistics are presented.

There is also an implementation of the SIGINT signal to be detected by the _sauna_ program to end both programs.

In the file _ger.pid_, where pid is the process ID, there are several information regarding the program's execution. Each line is in the format, inst – pid – tid – p: g – dur – tip, for each request.

### Sauna Program

_Invocation:_ ./sauna <number of seats>
 
The program prints, initially, the initial queue and, for each change, it prints the new queue, like entrances, rejections, exits, if the queue is empty, etc.
In the end, some statistics are presented.


In the file _bal.pid_, where pid is the process ID, there are several information regarding the program's execution. Each line is in the format, inst – pid – tid – p: g – dur – tip, for each request.


Durante o desenvolvimento do programa sauna.c, fomos deparados com algumas situações de competição no que toca a acesso a elementos partilhados como os lugares na sauna e o número de pessoas dentro da sauna.
Para resolver o acesso aos lugares, foi criado um array de semáforos, de modo a impedir acesso a um lugar que já estivesse ocupado. 
O número de pessoas que estão na sauna diminui no fim de cada thread criada para um certo pedido. Para resolver possíveis race conditions, foi criado um mutex que bloqueia antes da decrementação do número de pessoas e desbloqueia após a dita decrementação.

## Race Conditions

During the development of the _sauna.c_ file, there was some competition regarding access to the shared elements, like sauna seats and number of people inside the sauna.

* To fix the first competition, it was created an array of semaphores.
* To fix the second competition, the number of people that are in the sauna are reduced in the end of each thread. To fix possible race conditions, a mutex was created that locks before the decrement of number of people and unlocks after the said decrement.

## Authors

|Name               | Number    | Profile
| ------------- |:-------------:| --------:|
|João Santos       | 201504013 | [jotadaxter](https://github.com/jotadaxter)
|José Azevedo      | 201506448 | [zemafaz](https://github.com/zemafaz)
|Vitor Magalhães    | 201503447 | [LastLombax](https://github.com/LastLombax)

