# Implementação: echo UART pelo BLE, com conversão de letras minúsculas para maiúsculas.

A implementação foi realizada utilizando o [Zephyr 3.2.99](https://docs.zephyrproject.org/latest/) e se dá em duas partes:

- Implementação de um dispositivo periférico que deve:
    - ter uma característica com permissão de escrita e que irá receber os dados
    - ter uma característica sem permissão e que realiza notificação
    - a notificação deverá ser enviada sempre que um dado for recebido
    - o conteúdo da notificação é o dado recebido com as letras minúsculas convertidas para maiúsculas

- Implementação de um dispositivo central que deve:
    - procurar e se conectar ao periférico
    - enviar ao periférico o que o usuário escreve no terminal
    - imprimir a resposta enviada pelo periférico

A simulação foi realizada utilizando o [Renode](https://renode.io/). Fazendo uso de um [script](https://github.com/renode/renode/blob/master/scripts/multi-node/nrf52840-ble-zephyr.resc) para criar uma rede BLE com dois dispositivos.
