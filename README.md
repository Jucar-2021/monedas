💡 Descripción del Proyecto

Sistema de Control de Monedero Electrónico con Arduino Mega 2560
Este proyecto implementa un sistema de detección y control de créditos basado en monedas, utilizando un selector de monedas HX-916 y una tarjeta Arduino Mega 2560. El sistema identifica los pulsos eléctricos generados por cada moneda, acumula el valor depositado y activa una bobina o relevador de potencia una vez alcanzado un monto equivalente a $5.00 MXN.

El sistema emplea un display LCD I2C (20x4) para la interacción con el usuario, y salida en el Monitor Serial para diagnóstico en tiempo real.
Además, puede integrarse con una tarjeta intermedia que regula y separa los niveles de voltaje de potencia (110 V AC) y de control lógico (5 V DC / 12 V DC).

⚙️ Funcionalidad Principal

Detección de pulsos provenientes del monedero (pin D15).

Mapeo de monedas:

2 cambios = $1.00

4 cambios = $2.00

6 o más cambios = $5.00 (activa la bobina de inmediato)

Acumulación de pulsos: al llegar a 10 cambios (equivalente a $5), se activa la bobina.

Activación de bobina (pin D11) durante 5 segundos, controlada mediante relé o SSR.

Visualización en LCD de:

Moneda detectada

Monto depositado actual

Cuenta regresiva durante la activación.

Salida de diagnóstico por Monitor Serial, mostrando modo, cambios detectados, valor acumulado y eventos de activación.

🧠 Características Técnicas

Microcontrolador principal: Arduino Mega 2560

Pantalla: LCD 20x4 con interfaz I2C

Monedero compatible: HX-916 (TTL a 12 V) u otros con salida de pulsos

Voltaje de control lógico: 5 V DC (Arduino)

Voltaje auxiliar de alimentación: 12 V DC (para monedero y módulos intermedios)

Voltaje de salida de potencia: 110 V AC (bobina / carga controlada mediante relé o SSR)

Tiempo de activación de bobina: 5 segundos

Comunicación Serial: 115200 baud (para diagnóstico)

Compatibilidad con tarjeta intermedia:

Módulos reguladores DC-DC para aislar tensiones

Relés o SSR para manejo de cargas de corriente alterna

Fuente de alimentación conmutada o transformador independiente para evitar interferencias eléctricas

🧩 Lógica de Operación

El sistema detecta los pulsos generados por la inserción de una moneda.

Según el número de pulsos, determina la denominación.

Se acumulan los valores hasta completar $5.00 MXN.

Al alcanzar o superar este valor:

Se activa la bobina o relé durante 5 segundos.

Se reinicia el contador de cambios y el total depositado.

Durante el proceso, la información se muestra en el LCD y se registra en el Monitor Serial.

⚡ Seguridad y Aislamiento

Para evitar daños en la placa Arduino y garantizar seguridad eléctrica:

Se recomienda utilizar optocopladores o relés SSR con aislamiento galvánico.

Implementar una fuente de alimentación conmutada de doble salida:

12 V DC para el monedero.

5 V DC regulado para la lógica de control.

Mantener la línea de 110 V AC totalmente aislada del circuito de control.

👨‍💻 Autor

Desarrollador: Juan Carlos García Lucero
📞 Teléfono: 773126961
📍 Ubicación: Hidalgo, México

🪛 Licencia

Este proyecto es de libre uso para fines educativos y experimentales.
Se permite su modificación y distribución, siempre que se mantenga el crédito al autor original.
