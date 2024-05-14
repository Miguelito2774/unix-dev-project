# Servidor HTTP

Este repositorio contiene el proyecto final de desarrollo Unix, es un servidor HTTP y sus instrucciones para poder hacerlo funcionar..

## Dependencias

- **Compilador de C:** Asegúrate de tener un compilador de C instalado en tu sistema. Puedes instalar GCC en sistemas basados en Unix o MinGW en Windows.
- **Python:** El servidor hace uso de Python para algunas tareas. Asegúrate de tener Python instalado en tu sistema.
- **Herramienta para probar solicitudes:** Se recomienda el uso de Postman u otra herramienta similar para probar las solicitudes al servidor.

## Instrucciones de Ejecución

1. Clona este repositorio en tu máquina local:

    ```bash
    git clone https://ruta-del-repositorio
    ```

2. Navega al directorio del proyecto:

    ```bash
    cd unix-dev-project
    ```

3. Compila el código fuente del servidor:

    ```bash
    gcc -o ./out ./main.c
    ```

4. Ejecuta el servidor especificando el puerto en el que deseas que el servidor escuche, por ejemplo, el puerto 8080:

    ```bash
    ./out -p 8080
    ```

5. Si ves el mensaje "Server is running on port 8080", entonces el servidor se ha iniciado correctamente.

## Probando el Servidor

Una vez que el servidor esté en funcionamiento, puedes probar las solicitudes utilizando herramientas como Postman. Aquí hay algunas solicitudes que puedes probar:

- **GET Request a python.py:**

    - URL: `http://localhost:8080/python.py`
    - Descripción: Esta solicitud devuelve el código fuente del archivo `python.py` si se realiza una solicitud GET. Si se realiza una solicitud POST, devuelve la ejecución del código Python contenido en `python.py`.

- **POST Request a python.py:**

    - URL: `http://localhost:8080/python.py`
    - Descripción: Esta solicitud ejecuta el código Python contenido en `python.py` y devuelve el resultado de la ejecución.

- **GET Request a index.html:**

    - URL: `http://localhost:8080/index.html`
    - Descripción: Esta solicitud devuelve el contenido del archivo `index.html` si se realiza una solicitud GET.

## Detener el Servidor

Para detener el servidor, simplemente presiona `Ctrl + C` en la terminal donde se está ejecutando el servidor. Verás un mensaje indicando que el servidor se está apagando.

