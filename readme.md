
# 🌐 Sistema de Monitoramento de Distância com Arduino e Ethernet

Projeto de automação embarcada utilizando **Arduino MEGA**, **sensor ultrassônico HC-SR04** e **módulo Ethernet W5100**.  
O sistema mede a distância de objetos em tempo real e envia os dados para um servidor web via **protocolo **MQTT**.

---

## 🧠 Sobre o Projeto

O sistema foi desenvolvido com o objetivo de **monitorar distâncias sobre um produto** e **enviar os dados automaticamente para um broker MQTT**, permitindo o acompanhamento remoto.

A comunicação é feita através do módulo **Ethernet W5100**, conectado à rede local, enviando leituras para um **broker MQTT**.

Para a aplicação local sem levar em conta a comunicação de dados, temos leds que possuem comportamentos conforma as regras definidas para aprovação ou reprovação do produto.

---

## Bibliotecas utilizadas

- PubSubClient.h
- Ethernet.h
- LiquidCrystal.h
- ArduinoJson.h
- NTPClient.h

## 🧩 Instruções de Execução

Realizar o clone deste repositório, o upload do codigo fonte diretamente para o arduino.
Com a placa Ethernet já conectada na arduino e conectada em rede, ao realizar o uploading do fonte para a placa, a conexão com a rede já deve ser realizada de forma automática, buscando o DHCP de rede.

- Para configuração do broker MQTT, se torna necessário a alteração do servidor de envio pela constante de sistema MQTT_SERVER.

---

## 🔌 Topicos de disparo MQTT

- iot/riodosul/si/BSN22025T26F8/cell/09/device/c09-ayrton-djonatan/cmd 

Espera RECEBER um Json conforme a baixo:
```json
{"action": "get_status"}
```
- iot/riodosul/si/BSN22025T26F8/cell/09/device/c09-ayrton-djonatan/telemetry

Envia um Json conforme a baixo:
``` json
{
  "ts": "1951",
  "ts": "1958",
  "cellId": 9,
  "devId": "c09-ayrton-djonatan",
  "metrics": {
    "dist_cm": 34,
    "qualidade": "Reprovado"
  },
  "status": "Reprovado",
  "units": "cm",
  "thresholds": {
    "min_cm": 5,
    "max_cm": 7
  }
}
```
- iot/riodosul/si/BSN22025T26F8/cell/09/device/c09-ayrton-djonatan/state

Dispara a informação: ON, Off

- iot/riodosul/si/BSN22025T26F8/cell/09/device/c09-ayrton-djonatan/event

Dispara os eventos de alteração de status do produto:
```json
{
    "ts": 135,
    "type": "peca_aprovada|peca_reprovada",
    "info": "Aprovado|Reprovado"
}
```

## 🧰 Componentes Utilizados

| Componente | Função |
|-------------|--------|
| Arduino MEGA | Microcontrolador principal |
| Sensor Ultrassônico HC-SR04 | Medição de distância |
| Módulo Ethernet W5100 | Conexão de rede (IP fixo ou DHCP) |
| Cabo RJ-45 | Comunicação via rede local |
| Jumpers e Protoboard | Conexões elétricas |

---

## ⚙️ Funcionalidades

- 🔍 Leitura contínua da distância em centímetros  
- 🌐 Envio dos dados via MQTT
- 💡 Reconexão automática à rede em caso de falha  
- 🧭 Log serial para depuração e diagnóstico  

---

## 👤 Autores

Djonatan Oliveira e Ayrton Klettenberg



## 📜 Licença

Este projeto está sob a licença MIT
