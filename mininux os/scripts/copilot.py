#!/usr/bin/env python3
"""Small native desktop client for the OpenAI Responses API."""

import json
import os
import queue
import sys
import threading
import urllib.error
import urllib.request


API_URL = "https://api.openai.com/v1/responses"
DEFAULT_MODEL = "gpt-4o-mini"
SYSTEM_MESSAGE = (
    "Eres Minux Copilot. Responde en español de forma clara y útil. "
    "Minux OS es un prototipo educativo; no afirmes que tiene funciones "
    "que todavía no están implementadas."
)
MAX_HISTORY_MESSAGES = 12


class CopilotError(Exception):
    """An expected API or connectivity error safe to show in the UI."""


def _response_text(response):
    parts = []
    for item in response.get("output", []):
        for content in item.get("content", []):
            if content.get("type") == "output_text":
                parts.append(content.get("text", ""))
    text = "\n".join(part for part in parts if part).strip()
    if not text:
        raise CopilotError("La API no devolvió texto. Inténtalo de nuevo.")
    return text


def _error_message(body):
    try:
        data = json.loads(body)
    except json.JSONDecodeError:
        return ""
    error = data.get("error", {})
    if isinstance(error, dict):
        return str(error.get("message", ""))
    return ""


def request_reply(messages, api_key, model):
    payload = json.dumps(
        {"model": model, "input": messages, "store": False}
    ).encode("utf-8")
    request = urllib.request.Request(
        API_URL,
        data=payload,
        headers={
            "Authorization": "Bearer " + api_key,
            "Content-Type": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            body = response.read().decode("utf-8")
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        message = _error_message(detail)
        raise CopilotError(
            message or "La API respondió con HTTP {}.".format(error.code)
        ) from error
    except urllib.error.URLError as error:
        raise CopilotError(
            "No se pudo conectar. Comprueba la Wi-Fi y el acceso a Internet."
        ) from error

    try:
        data = json.loads(body)
    except json.JSONDecodeError as error:
        raise CopilotError("La API devolvió una respuesta JSON inválida.") from error
    return _response_text(data)


class CopilotApp:
    def __init__(self, root, api_key, model):
        import tkinter as tk
        from tkinter import ttk

        self.root = root
        self.api_key = api_key
        self.model = model
        self.history = []
        self.results = queue.Queue()
        self.busy = False

        root.title("Minux Copilot")
        root.geometry("760x560")
        root.minsize(520, 380)

        container = ttk.Frame(root, padding=12)
        container.pack(fill="both", expand=True)
        self.transcript = tk.Text(
            container, wrap="word", state="disabled", height=20
        )
        scrollbar = ttk.Scrollbar(
            container, orient="vertical", command=self.transcript.yview
        )
        self.transcript.configure(yscrollcommand=scrollbar.set)
        self.transcript.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        self.transcript.tag_configure("user", foreground="#1457a6")
        self.transcript.tag_configure("assistant", foreground="#176b3a")
        self.transcript.tag_configure("error", foreground="#a32626")

        controls = ttk.Frame(root, padding=(12, 0, 12, 12))
        controls.pack(fill="x")
        self.prompt = ttk.Entry(controls)
        self.prompt.pack(side="left", fill="x", expand=True)
        self.prompt.bind("<Return>", self._submit)
        self.send_button = ttk.Button(
            controls, text="Enviar", command=self._submit
        )
        self.send_button.pack(side="left", padx=(8, 0))
        self.status = ttk.Label(root, text="Conexión a través de Internet del PC")
        self.status.pack(anchor="w", padx=12, pady=(0, 8))
        self._append(
            "Minux Copilot listo. La conversación se conserva solo en memoria "
            "mientras esta ventana está abierta.\n",
            "assistant",
        )

    def _append(self, text, tag=None):
        self.transcript.configure(state="normal")
        self.transcript.insert("end", text, tag)
        self.transcript.see("end")
        self.transcript.configure(state="disabled")

    def _submit(self, _event=None):
        text = self.prompt.get().strip()
        if not text or self.busy:
            return "break"

        self.prompt.delete(0, "end")
        self._append("Tú: " + text + "\n", "user")
        conversation = [
            {"role": "system", "content": SYSTEM_MESSAGE}
        ] + (self.history + [{"role": "user", "content": text}])[
            -MAX_HISTORY_MESSAGES:
        ]
        self.busy = True
        self.send_button.configure(state="disabled")
        self.status.configure(text="Esperando respuesta…")
        threading.Thread(
            target=self._request_in_background,
            args=(conversation, text),
            daemon=True,
        ).start()
        self.root.after(100, self._poll_result)
        return "break"

    def _request_in_background(self, conversation, user_text):
        try:
            answer = request_reply(conversation, self.api_key, self.model)
            self.results.put(("success", user_text, answer))
        except CopilotError as error:
            self.results.put(("error", user_text, str(error)))

    def _poll_result(self):
        try:
            result, user_text, message = self.results.get_nowait()
        except queue.Empty:
            self.root.after(100, self._poll_result)
            return

        self.busy = False
        self.send_button.configure(state="normal")
        if result == "success":
            self.history.extend(
                [
                    {"role": "user", "content": user_text},
                    {"role": "assistant", "content": message},
                ]
            )
            self.history = self.history[-MAX_HISTORY_MESSAGES:]
            self._append("Copilot: " + message + "\n\n", "assistant")
            self.status.configure(text="Listo")
        else:
            self._append("Error: " + message + "\n\n", "error")
            self.status.configure(text="No se pudo completar la solicitud")


def main():
    api_key = os.environ.get("OPENAI_API_KEY", "").strip()
    if not api_key:
        print(
            "Configura OPENAI_API_KEY en el entorno antes de iniciar Copilot.",
            file=sys.stderr,
        )
        return 1

    model = os.environ.get("OPENAI_MODEL", DEFAULT_MODEL).strip()
    if not model:
        model = DEFAULT_MODEL
    try:
        import tkinter as tk
    except ModuleNotFoundError as error:
        if error.name != "tkinter":
            raise
        print(
            "Falta Tkinter. En Ubuntu/Debian, instala python3-tk.",
            file=sys.stderr,
        )
        return 1
    root = tk.Tk()
    CopilotApp(root, api_key, model)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
