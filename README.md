# Own Workqueue

Vamos criar nossa própria Workqueue no Driver de Dispositivos Linux 

## Começando

Primeiro, limpamos os logs do Dmesg:
```bash
sudo dmesg -c
```

Compile o driver utilizando o Makefile:

```bash
sudo make
```
Carregue o driver utilizando:

```bash
sudo insmod driver.ko
```

Para acionar o arquivo de dispositivo de leitura de interrupção: 

```bash
sudo cat /dev/etx_device
```

Agora observamos o Dmesg utilizando:
```bash
dmesg
```

Caso encontre problemas em relação a permissões com o DMesg, utilize:
```bash
sudo sysctl kernel.dmesg_restrict=0
```


## Saídas

```python
[ 6842.629534] Major = 510 Minor = 0 
[ 6842.630099] Insercao do Driver do Dispositivo...Feito!!!
[ 6849.413361] Arquivo do Dispositivo Aberto...!!!
[ 6849.413398] Funcao de Leitura
[ 6849.413403] Shared IRQ: Interrupt Occurred
[ 6849.413426] Executando a Funcao Workqueue
[ 6849.413438] Arquivo do Dispositivo Fechado..!!!
```

Você pode também ver sua fila "own_wq" utilizando o comando:
```bash
ps -aef
```
```bash
UID          PID    PPID  C STIME TTY          TIME CMD
root       12550       2  0 21:07 ?        00:00:00 [own_wq]
```

Por último, descarregue o módulo utilizando: 
```bash
sudo rmmod driver
```

## Fontes usadas:

Você pode encontrar mais sobre o desenvolvimento deste projeto no tutorial a seguir:
[EmbeTronicX -> Device Driver 16 - Own Workqueue](https://embetronicx.com/tutorials/linux/device-drivers/work-queue-in-linux-own-workqueue/)
