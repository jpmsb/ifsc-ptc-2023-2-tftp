# Cliente TFTP (versão 2)

Esse cliente TFTP possui as seguintes classes:

- RRQ
- WRQ
- DATA
- ACK
- ERROR
- TFTP
- Mensagem

No processo de lógica, é utilizada a máquina de estados finita comunicante abaixo:

![Máquina de estados finita comunicante do servidor TFTP](../imagens/maquinas-de-estado/cliente.png)

Assim como realizado no servidor, a implementação da máquina de estados acima em *software* foi realizada utilizando utilizando as classes *Poller* e *Callback*, sendo criada a classe *TFTPServer* como uma especialização da classe *Callback*.

O laço que faz a máquina de estados funcionar é orquestrado pelo *Poller*, que chamada um método interno denominado `handle()`. É dentro deste método que a máquina de estados é implementada. O método `handle()` é chamado a cada iteração do descritor do *socket* UDP, que é o meio pelo qual as informações são transportadas.

Para tratar o *timeout*, é utilizado o método do *Poller* denominado `handle_timeout()`, que é chamado quando o *timeout* é atingido. Este método repete a última operação que exija resposta do cliente. O limite definido internamente é de três tentativas.

Além das mensagens padrão do protocolo TFTP, foram implementadas novas mensagens:

- list
- move
- mkdir

Tais mensagens foram implementadas utilizando o protocolo ProtoBuffers, que é um protocolo de serialização de dados binários. Todas operações que envolvem essas mensagens são realizadas utilizando os recursos do ProtoBuffers.

## Estados

Há quatro estados possíveis para o servidor:

 - Início
 - Transmitir
 - Receber
 - Resultado

### Início

Estado em que a operação é definida. Essa operação vem do programa cliente que usa a classe TFTP. As operações são definidas em uma enum pública:

```c++
enum Operation {
    SEND,
    RECEIVE,
    LIST,
    MKDIR,
    MOVE
};
```

O uso da Enum é útil, principalmente com a implementação das novas mensagens em ProtoBuffers, pois permite diferenciar o tipo de tratamento em um determinado estado. Como é o exemplo na operação de listagem de arquivos, que é diferente da operação de recebimento de arquivos, porém compartilham o estado "Receber".

A enum que seleciona a operação é obtida na máquina de estados como um parâmetro do construtor da classe TFTP:

```c++
TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, Operation operation, string & sourceFile, string & destinationFile);
```

#### RRQ

Ocorre quando a operação selecionada é "RECEIVE".

O cliente envia uma mensagem RRQ para o servidor. O estado é alterado para "Receber" e o "timeout" é ativado. Na próxima iteração do servidor para com o cliente, este entrará no estado "Receber".

#### WRQ

Ocorre quando a operação selecionada é "SEND".

O cliente envia uma mensagem WRQ para o servidor. O estado é alterado para "Transmitir" e o "timeout" é ativado. Na próxima iteração do servidor para com o cliente, este entrará no estado "Transmitir".

#### LIST

Ocorre quando a operação selecionada é "LIST".

Uma mensagem ProtoBuffers é criada e serializada da seguinte forma:

```c++
pbMessage = new tftp2::Mensagem();
tftp2::PATH* listMessage = pbMessage->mutable_list();
listMessage->set_path(srcFile);
string serializedMessage = pbMessage->SerializeAsString();
sock.send(serializedMessage.c_str(), serializedMessage.size(), addr);
```

O *timeout* é ativado e o estado é alterado para "Receber". Na próxima iteração do servidor para com o cliente, este entrará no estado "Receber".

#### MKDIR

Ocorre quando a operação selecionada é "MKDIR".

Uma mensagem ProtoBuffers é criada e serializada da seguinte forma:

```c++
pbMessage = new tftp2::Mensagem();
tftp2::PATH* mkdirMessage = pbMessage->mutable_mkdir();
mkdirMessage->set_path(srcFile);
string serializedMessage = pbMessage->SerializeAsString();
```

O *timeout* é ativado e o estado é alterado para "Receber". Na próxima iteração do servidor para com o cliente, este entrará no estado "Resultado".

#### MOVE

Ocorre quando a operação selecionada é "MOVE".

Uma mensagem ProtoBuffers é criada e serializada da seguinte forma:

```c++
pbMessage = new tftp2::Mensagem();
tftp2::PATH* moveMessage = pbMessage->mutable_move();
moveMessage->set_old_name(srcFile);
moveMessage->set_new_name(destFile);
serializedMessage = pbMessage->SerializeAsString();
```

O *timeout* é ativado e o estado é alterado para "Receber". Na próxima iteração do servidor para com o cliente, este entrará no estado "Resultado".


### Transmitir

Ocorre quando o cliente envia um arquivo para o servidor. O arquivo é enviado em pacotes de 512 bytes. O cliente envia um pacote de dados e aguarda um ACK do servidor. Caso o ACK não chegue, o cliente reenvia o pacote de dados. O cliente reenvia o pacote de dados até que o ACK chegue ou limite de tentativas seja atingido.

Um pacote com dados menos do que 512 bytes é considerado o último pacote e finaliza a transmissão e o ciclo da máquina de estados. Para tal, usa-se o método `finish()` do Poller.

### Receber

Ocorre quando o cliente recebe dados do servidor. Nesse caso, esses dados podem ser partes de um arquivo ou de uma *string* ProtoBuffers serializada, que vem da operação LIST.

Ao sair do estado "Inicio", o cliente recebe um pacote DATA e entra no estado "Receber". O cliente envia um ACK para o servidor e aguarda um novo pacote DATA. Caso o pacote DATA não chegue, o cliente reenvia o ACK. O cliente reenvia o ACK até que o pacote DATA chegue ou limite de tentativas seja atingido.

Um pacote com dados menos do que 512 bytes é considerado o último pacote e finaliza a transmissão e o ciclo da máquina de estados. Para tal, usa-se o método `finish()` do Poller.

### Resultado

Ocorre nas operações de MOVE e MKDIR. Os tipos de mensagem recebida podem ser ACK0 ou ERROR.

## Classes

As classes mencionadas abaixo são muito similares às usadas na implementação do servidor.

### RRQ

Classe que representa uma mensagem RRQ. Possui os seguintes atributos:

```c++
uint16_t opcode;
vector<char> filename;
vector<char> mode;
int opcodeSize = 2;
int filenameSize;
int modeSize;
```

O construtor recebe uma *string* com o nome do arquivo e uma *string* com o modo de operação. 


### WRQ

Classe que representa uma mensagem WRQ. Possui os seguintes atributos:

```c++
uint16_t opcode;
vector<char> filename;
vector<char> mode;
int opcodeSize = 2;
int filenameSize;
int modeSize;
```

O construtor recebe uma *string* com o nome do arquivo e uma *string* com o modo de operação. É praticamente igual a classe RRQ, porém com o opcode diferente.


### DATA

Classe que representa uma mensagem DATA. Possui os seguintes atributos:

```c++
uint16_t opcode;
uint16_t block;
char data[512];
int count;
std::ifstream file;
std::streamsize bytesAmount;
```

Há três construtores diferentes:

- Vazio
  
```c++
DATA();
```

- Recebe uma string com o nome do arquivo. Usado quando o cliente envia um arquivo para o servidor.
  
```c++
DATA(std::string & filename);
```

- Recebe um vetor de char com os dados e o tamanho do vetor. Usado quando o cliente recebe dados do servidor.
  
```c++
DATA(char bytes[], size_t size);
```

### ACK

Classe que representa uma mensagem ACK. Possui os seguintes atributos:

```c++
uint16_t opcode;
uint16_t block;
```

Há dois construtores diferentes:

- Vazio
  
```c++ 
ACK();
```

- Recebe um array de char. Usado para interpretar um pacote recebido do servidor.
  
```c++
ACK(char bytes[]);
```

### ERROR

Classe que representa uma mensagem ERROR. Possui os seguintes atributos:

```c++
uint16_t opcode;
uint16_t errorCode;
std::string errorMessage;
```

Há dois construtores diferentes:

- Vazio
  
```c++
ERROR();
```

- Recebe um arranjo de char e o tamanho do arranjo. Usado para interpretar um pacote recebido do servidor.
  
```c++
ERROR(char bytes[], size_t size);
```

A diferença dessa classe para a do servidor, é que aqui não há a definição das mensagens de erro, servindo para justamente receber o pacote de erro do servidor e tratá-lo de forma mais facilitada.

### TFTP

Classe que representa o servidor TFTP.

Possui os seguintes atributos:

```c++
void handle();
void handle_timeout();
void start();
enum Estado {
    Conexao,
    Transmitir,
    Receber,
    Resultado,
    Fim
};
Estado estado;
sockpp::UDPSocket & sock;
sockpp::AddrInfo addr;
Operation operation;
string srcFile;
string destFile;
RRQ * rrq;
WRQ * wrq;
DATA * data;
ACK * ack;
ERROR * error;
ofstream * outputFile;
char buffer[516];
int bytesAmount;
uint8_t timeoutCounter;
bool timeoutState;
tftp2::Mensagem * pbMessage;
string serializedMessage;
string listingOutput;
```

O construtor é declarado da seguinte forma:

```c++
TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, Operation operation, string & sourceFile, string & destinationFile);
```

O método `handle()` é o responsável por orquestrar a máquina de estados. É chamado a cada iteração do Poller.

O método `handle_timeout()` é o responsável por tratar o timeout. É chamado quando o timeout é atingido.