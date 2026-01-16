import sys
import ctypes
import os
import keyboard
import uuid
import base64
from PySide6.QtWidgets import (QApplication, QWidget, QVBoxLayout, QHBoxLayout, 
                             QLineEdit, QTextEdit, QLabel, QPushButton, QListWidget, 
                             QCheckBox, QDialog, QColorDialog, QFontComboBox, QSpinBox,
                             QGroupBox, QGridLayout, QTabWidget, QKeySequenceEdit, QFrame,
                             QListWidgetItem, QComboBox, QFileDialog, QTextBrowser)
from PySide6.QtCore import Qt, QTimer, QEvent, Signal, QSize, QBuffer, QByteArray, QIODevice, QUrl
from PySide6.QtGui import QKeySequence, QIcon, QPixmap, QImage, QDesktopServices

if hasattr(sys, '_MEIPASS'):
    base_path = sys._MEIPASS
else:
    base_path = os.path.dirname(os.path.abspath(__file__))

icos_path = os.path.join(base_path, "icos")
dll_path = os.path.join(base_path, "backend.dll")

if not os.path.exists(dll_path):
    print(f"backend.dll missing {dll_path}")
    sys.exit()

lib = ctypes.CDLL(dll_path)
lib.start_host.argtypes = [ctypes.c_int, ctypes.c_char_p]
lib.start_join.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p]
lib.send_msg.argtypes = [ctypes.c_char_p]
lib.poll.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
lib.set_spam.argtypes = [ctypes.c_bool]

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

SW_RESTORE = 9

class SettingsWindow(QDialog):
    def __init__(self, parent_ref):
        super().__init__(None)
        self.mw = parent_ref
        self.old_pos = None
        
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.WindowSystemMenuHint)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.resize(350, 450)
        
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(0)
        
        self.frame_header = QFrame()
        self.frame_header.setFixedHeight(35)
        self.header_layout = QHBoxLayout(self.frame_header)
        self.header_layout.setContentsMargins(10, 0, 10, 0)
        
        self.lbl_title = QLabel("Settings")
        self.lbl_title.setStyleSheet("color: white; font-weight: bold; border: none; background: transparent;")
        
        self.btn_close = QPushButton("X")
        self.btn_close.setFixedSize(25, 20)
        self.btn_close.setStyleSheet("background-color: #cc0000; border: none; color: white; font-weight: bold;")
        self.btn_close.clicked.connect(self.accept)
        
        self.header_layout.addWidget(self.lbl_title)
        self.header_layout.addStretch()
        self.header_layout.addWidget(self.btn_close)
        
        self.frame_body = QFrame()
        self.body_layout = QVBoxLayout(self.frame_body)
        self.body_layout.setContentsMargins(10, 10, 10, 10)
        
        self.main_layout.addWidget(self.frame_header)
        self.main_layout.addWidget(self.frame_body)
        
        header_style = """
            QFrame {
                background-color: #333333;
                border-top-left-radius: 8px;
                border-top-right-radius: 8px;
                border: 1px solid #555;
                border-bottom: none;
            }
        """
        body_style = """
            QFrame {
                background-color: rgba(25, 25, 25, 240);
                border: 1px solid #555;
                border-top: none;
                border-bottom-left-radius: 8px;
                border-bottom-right-radius: 8px;
            }
        """
        self.frame_header.setStyleSheet(header_style)
        self.frame_body.setStyleSheet(body_style)
        
        self.setStyleSheet("""
            QTabWidget::pane { border: 0; }
            QTabBar::tab { background: #444; color: #ccc; padding: 5px 10px; margin-right: 2px; }
            QTabBar::tab:selected { background: #666; color: white; font-weight: bold; }
            QLabel { color: white; background: transparent; }
            QCheckBox { color: white; background: transparent; }
            QLineEdit { background: rgba(0,0,0,150); color: white; border: 1px solid #555; padding: 2px; }
            QPushButton { background: #444; color: white; border: 1px solid #555; padding: 3px; }
            QSpinBox { background: rgba(0,0,0,150); color: white; border: 1px solid #555; }
            QFontComboBox { background: rgba(0,0,0,150); color: white; border: 1px solid #555; }
            QComboBox { background: rgba(0,0,0,150); color: white; border: 1px solid #555; padding: 2px; }
            QComboBox::item { color: white; background: #333; }
        """)

        tabs = QTabWidget()
        
        tab_gen = QWidget()
        l_gen = QVBoxLayout()
        
        chk_spam = QCheckBox("Anti-Spam")
        chk_spam.setChecked(True)
        chk_spam.toggled.connect(lambda c: lib.set_spam(c))
        l_gen.addWidget(chk_spam)
        
        chk_pin = QCheckBox("Always on Top")
        chk_pin.setChecked(self.mw.pinned)
        def toggle_pin(c):
            self.mw.pinned = c
            flags = self.mw.windowFlags()
            if c: flags |= Qt.WindowStaysOnTopHint
            else: flags &= ~Qt.WindowStaysOnTopHint
            self.mw.setWindowFlags(flags)
            self.mw.show()
        chk_pin.toggled.connect(toggle_pin)
        l_gen.addWidget(chk_pin)

        chk_ghost = QCheckBox("Ghost Mode")
        chk_ghost.setChecked(self.mw.ghost_mode)
        def toggle_ghost(c):
            self.mw.ghost_mode = c
            self.mw.update_style(True)
        chk_ghost.toggled.connect(toggle_ghost)
        l_gen.addWidget(chk_ghost)
        l_gen.addStretch()
        tab_gen.setLayout(l_gen)

        tab_prof = QWidget()
        l_prof = QVBoxLayout()
        l_prof.addWidget(QLabel("Select your Avatar:"))
        
        cmb_avatar = QComboBox()
        cmb_avatar.setIconSize(QSize(24, 24))
        
        colors = ["orange", "red", "blue", "green"]
        for c in colors:
            icon_path = os.path.join(icos_path, f"{c}.ico")
            if os.path.exists(icon_path):
                cmb_avatar.addItem(QIcon(icon_path), c.capitalize(), c)
            else:
                cmb_avatar.addItem(c.capitalize(), c)
        
        idx = cmb_avatar.findData(self.mw.my_icon)
        if idx >= 0: cmb_avatar.setCurrentIndex(idx)
        
        def update_icon(idx):
            self.mw.my_icon = cmb_avatar.currentData()
            
        cmb_avatar.currentIndexChanged.connect(update_icon)
        l_prof.addWidget(cmb_avatar)
        l_prof.addWidget(QLabel("(Reconnect to update for others)"))
        l_prof.addStretch()
        tab_prof.setLayout(l_prof)
        
        tab_app = QWidget()
        grid = QGridLayout()
        
        def add_color_row(row, label_text, var_name):
            lbl = QLabel(label_text)
            txt = QLineEdit(getattr(self.mw, var_name))
            btn = QPushButton("Pick")
            def pick_color():
                c = QColorDialog.getColor()
                if c.isValid():
                    hex_code = c.name()
                    setattr(self.mw, var_name, hex_code)
                    txt.setText(hex_code)
            def text_changed(val): setattr(self.mw, var_name, val)
            btn.clicked.connect(pick_color)
            txt.textChanged.connect(text_changed)
            grid.addWidget(lbl, row, 0)
            grid.addWidget(txt, row, 1)
            grid.addWidget(btn, row, 2)

        add_color_row(0, "My Name:", "col_me")
        add_color_row(1, "Friend Name:", "col_friend")
        add_color_row(2, "Message:", "col_text")
        
        grid.addWidget(QLabel("Font:"), 3, 0)
        font_box = QFontComboBox()
        font_box.setCurrentFont(self.mw.font())
        def change_font(f): self.mw.font_family = f.family()
        font_box.currentFontChanged.connect(change_font)
        grid.addWidget(font_box, 3, 1, 1, 2)
        
        grid.addWidget(QLabel("Size:"), 4, 0)
        spin_size = QSpinBox()
        spin_size.setValue(self.mw.font_size)
        def change_size(s): self.mw.font_size = s
        spin_size.valueChanged.connect(change_size)
        grid.addWidget(spin_size, 4, 1)
        
        tab_app.setLayout(grid)

        tab_keys = QWidget()
        l_keys = QVBoxLayout()
        
        l_keys.addWidget(QLabel("QuickerChatting key:"))
        key_edit = QLineEdit(self.mw.key_focus_chat)
        
        def update_key():
            new_key = key_edit.text()
            if new_key:
                self.mw.key_focus_chat = new_key
                self.mw.update_hotkey()
                
        key_edit.editingFinished.connect(update_key)
        l_keys.addWidget(key_edit)
        l_keys.addWidget(QLabel("Like: 'ctrl+space', 'insert', 'f1', 'alt+c'"))
        l_keys.addStretch()
        tab_keys.setLayout(l_keys)

        tabs.addTab(tab_gen, "General")
        tabs.addTab(tab_prof, "Profile")
        tabs.addTab(tab_app, "Style")
        tabs.addTab(tab_keys, "Keybinds")
        self.body_layout.addWidget(tabs)
        
    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.old_pos = event.globalPos()

    def mouseMoveEvent(self, event):
        if self.old_pos:
            delta = event.globalPos() - self.old_pos
            self.move(self.pos() + delta)
            self.old_pos = event.globalPos()

    def mouseReleaseEvent(self, event):
        self.old_pos = None

class ChatWindow(QWidget):
    focus_trigger = Signal()

    def __init__(self):
        super().__init__()
        
        self.pinned = False
        self.ghost_mode = True 
        self.is_focused = True
        self.settings_open = False
        
        self.col_me = "#00aaff"
        self.col_friend = "#ff4444"
        self.col_text = "#ffffff"
        self.font_family = "Segoe UI"
        self.font_size = 11

        self.key_focus_chat = "insert" 
        self.my_icon = "orange"
        
        self.img_buffer = None
        self.img_sender_name = ""
        self.image_map = {}
        
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.WindowSystemMenuHint)
        self.setAttribute(Qt.WA_TranslucentBackground)
        
        self.resize(600, 450)
        
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(0)
        
        self.frame_header = QFrame()
        self.frame_header.setFixedHeight(35)
        self.header_layout = QHBoxLayout(self.frame_header)
        self.header_layout.setContentsMargins(10, 0, 10, 0)
        
        self.lbl_title = QLabel("QuickerChatting")
        self.lbl_title.setStyleSheet("color: white; font-weight: bold; border: none; background: transparent;")
        self.btn_sets = QPushButton("Settings")
        self.btn_sets.setFixedSize(70, 20)
        self.btn_sets.setStyleSheet("background: #444; color: white; border: none;")
        self.btn_sets.clicked.connect(self.open_settings)
        self.btn_close = QPushButton("X")
        self.btn_close.setFixedSize(25, 20)
        self.btn_close.setStyleSheet("background-color: #cc0000; border: none; color: white; font-weight: bold;")
        self.btn_close.clicked.connect(self.close_app)
        
        self.header_layout.addWidget(self.lbl_title)
        self.header_layout.addStretch()
        self.header_layout.addWidget(self.btn_sets)
        self.header_layout.addWidget(self.btn_close)
        
        self.frame_body = QFrame()
        self.body_layout = QVBoxLayout(self.frame_body)
        self.body_layout.setContentsMargins(10, 10, 10, 10)
        
        self.conn_box = QHBoxLayout()
        self.ent_name = QLineEdit("MeandYou")
        self.ent_ip = QLineEdit("127.0.0.1")
        self.ent_port = QLineEdit("25565")
        self.btn_host = QPushButton("Host")
        self.btn_join = QPushButton("Join")
        
        style_input = "background: rgba(0,0,0,150); color: white; border: 1px solid #555; padding: 2px;"
        for w in [self.ent_name, self.ent_ip, self.ent_port]:
            w.setStyleSheet(style_input)
            self.conn_box.addWidget(w)
            
        btn_style = "background: #444; color: white; border: 1px solid #555; padding: 2px;"
        self.btn_host.setStyleSheet(btn_style)
        self.btn_join.setStyleSheet(btn_style)
        
        self.btn_host.clicked.connect(self.do_host)
        self.btn_join.clicked.connect(self.do_join)
        self.conn_box.addWidget(self.btn_host)
        self.conn_box.addWidget(self.btn_join)
        self.body_layout.addLayout(self.conn_box)

        self.chat_area = QHBoxLayout()
        self.txt_chat = QTextBrowser()
        self.txt_chat.setReadOnly(True)
        self.txt_chat.setOpenExternalLinks(False)
        self.txt_chat.setOpenLinks(False)
        self.txt_chat.setFocusPolicy(Qt.NoFocus) 
        self.txt_chat.anchorClicked.connect(self.handle_anchor)
        
        self.lst_users = QListWidget()
        self.lst_users.setFixedWidth(130)
        self.lst_users.setIconSize(QSize(20, 20))
        
        self.chat_area.addWidget(self.txt_chat)
        self.chat_area.addWidget(self.lst_users)
        self.body_layout.addLayout(self.chat_area)

        self.input_layout = QHBoxLayout()
        self.ent_msg = QLineEdit()
        self.ent_msg.setPlaceholderText("Type message...")
        self.ent_msg.setStyleSheet(style_input)
        self.ent_msg.returnPressed.connect(self.send_msg)
        
        self.btn_img = QPushButton("Img")
        self.btn_img.setFixedWidth(40)
        self.btn_img.setStyleSheet(btn_style)
        self.btn_img.clicked.connect(self.send_image)
        
        self.input_layout.addWidget(self.ent_msg)
        self.input_layout.addWidget(self.btn_img)
        self.body_layout.addLayout(self.input_layout)

        self.main_layout.addWidget(self.frame_header)
        self.main_layout.addWidget(self.frame_body)

        self.update_style(True)
        
        self.focus_trigger.connect(self.force_focus)
        self.update_hotkey()

        self.net_timer = QTimer()
        self.net_timer.timeout.connect(self.poll_backend)
        lib.init()
        self.net_timer.start(50)

        self.focus_timer = QTimer()
        self.focus_timer.timeout.connect(self.check_focus_state)
        self.focus_timer.start(100) 

        self.old_pos = None

    def handle_anchor(self, url):
        scheme = url.scheme()
        if scheme == "saveimg":
            img_id = url.path()
            if img_id in self.image_map:
                base64_data = self.image_map[img_id]
                try:
                    data = base64.b64decode(base64_data)
                    pix = QPixmap()
                    pix.loadFromData(data)
                    
                    file_path, _ = QFileDialog.getSaveFileName(self, "Save Image", "", "Images (*.jpg *.png)")
                    if file_path:
                        pix.save(file_path)
                except Exception as e:
                    print(f"Error saving image: {e}")
        else:
            QDesktopServices.openUrl(url)

    def update_hotkey(self):
        try:
            keyboard.unhook_all()
        except Exception:
            pass 

        if self.key_focus_chat:
            try:
                keyboard.add_hotkey(self.key_focus_chat, self.focus_trigger.emit)
            except Exception as e:
                print(f"Hotkey Error: {e}")

    def force_focus(self):
        hwnd = int(self.winId())
        foreground_hwnd = user32.GetForegroundWindow()
        
        current_thread_id = kernel32.GetCurrentThreadId()
        foreground_thread_id = user32.GetWindowThreadProcessId(foreground_hwnd, None)
        
        if current_thread_id != foreground_thread_id:
            user32.AttachThreadInput(foreground_thread_id, current_thread_id, True)
            user32.BringWindowToTop(hwnd)
            user32.ShowWindow(hwnd, SW_RESTORE)
            user32.AttachThreadInput(foreground_thread_id, current_thread_id, False)
        
        self.activateWindow()
        self.raise_()
        self.ent_msg.setFocus()
        self.ent_msg.selectAll()

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.old_pos = event.globalPos()

    def mouseMoveEvent(self, event):
        if self.old_pos:
            delta = event.globalPos() - self.old_pos
            self.move(self.pos() + delta)
            self.old_pos = event.globalPos()

    def mouseReleaseEvent(self, event):
        self.old_pos = None

    def check_focus_state(self):
        real_active = self.isActiveWindow()
        should_be_focused = real_active or self.settings_open or (not self.ghost_mode)
        
        if should_be_focused != self.is_focused:
            self.is_focused = should_be_focused
            self.update_style(should_be_focused)
        
        if not self.ghost_mode and not self.is_focused:
             self.is_focused = True
             self.update_style(True)

    def update_style(self, focused):
        if not self.ghost_mode:
            focused = True

        if focused:
            sb_style = """
                QScrollBar:vertical { width: 10px; background: transparent; margin: 0px; }
                QScrollBar::handle:vertical { background: #555; border-radius: 5px; min-height: 20px; }
                QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
                QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
            """
        else:
            sb_style = """
                QScrollBar:vertical { width: 0px; background: transparent; }
                QScrollBar::handle:vertical { background: transparent; }
                QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
            """

        if focused:
            header_style = """
                QFrame {
                    background-color: #333333;
                    border-top-left-radius: 8px;
                    border-top-right-radius: 8px;
                    border: 1px solid #555;
                    border-bottom: none;
                }
            """
            body_style = """
                QFrame {
                    background-color: rgba(25, 25, 25, 240);
                    border: 1px solid #555;
                    border-top: none;
                    border-bottom-left-radius: 8px;
                    border-bottom-right-radius: 8px;
                }
            """
            self.ent_msg.show()
            self.lst_users.show()
            for i in range(self.conn_box.count()):
                w = self.conn_box.itemAt(i).widget()
                if w: w.show()
            self.btn_sets.show()
            self.btn_close.show()
            self.lbl_title.show()
            self.btn_img.show()
        else:
            header_style = "background: transparent; border: none;"
            body_style = "background: transparent; border: none;"
            
            self.ent_msg.hide()
            self.lst_users.hide()
            for i in range(self.conn_box.count()):
                w = self.conn_box.itemAt(i).widget()
                if w: w.hide()
            self.btn_sets.hide()
            self.btn_close.hide()
            self.lbl_title.hide()
            self.btn_img.hide()

        self.frame_header.setStyleSheet(header_style)
        self.frame_body.setStyleSheet(body_style)
        
        chat_qss = f"""
            QTextBrowser {{
                background: transparent;
                border: none;
            }}
            {sb_style}
        """
        
        list_qss = f"""
            QListWidget {{
                background: rgba(0,0,0,100);
                color: #ddd;
                border: none;
            }}
            {sb_style}
        """

        self.txt_chat.setStyleSheet(chat_qss)
        self.lst_users.setStyleSheet(list_qss)

    def do_host(self):
        full_name = f"{self.ent_name.text()}|{self.my_icon}"
        lib.start_host(int(self.ent_port.text()), full_name.encode())
    
    def do_join(self):
        full_name = f"{self.ent_name.text()}|{self.my_icon}"
        lib.start_join(self.ent_ip.text().encode(), int(self.ent_port.text()), full_name.encode())

    def send_msg(self):
        t = self.ent_msg.text()
        if t:
            lib.send_msg(t.encode())
            self.ent_msg.clear()
            
    def send_image(self):
        file_name, _ = QFileDialog.getOpenFileName(self, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp)")
        if file_name:
            pix = QPixmap(file_name)
            if not pix.isNull():
                pix = pix.scaled(300, 300, Qt.KeepAspectRatio, Qt.SmoothTransformation)
                
                ba = QByteArray()
                buf = QBuffer(ba)
                buf.open(QIODevice.WriteOnly)
                pix.save(buf, "JPG", 70) 
                hex_data = ba.toBase64().data().decode()
                
                msg = f"[IMG]{hex_data}[/IMG]"
                lib.send_msg(msg.encode())

    def poll_backend(self):
        t = ctypes.create_string_buffer(64)
        d = ctypes.create_string_buffer(1024 * 1024) 
        
        count = 0
        while lib.poll(t, d) and count < 50:
            try:
                kind = t.value.decode()
                val = d.value.decode(errors='ignore')
            except Exception:
                continue

            if kind == "CHAT": 
                self.append_chat_msg(val)
            elif kind == "SYS": 
                self.txt_chat.append(f"<div style='color:orange; font-family:{self.font_family}; font-size:{self.font_size}pt;'>[SYS] {val}</div>")
            elif kind == "USERS":
                self.lst_users.clear()
                users = val.split(',')
                for u in users:
                    if not u: continue
                    if "|" in u:
                        name, color = u.split('|', 1)
                    else:
                        name, color = u, "orange"
                    
                    item = QListWidgetItem(name)
                    ic_path = os.path.join(icos_path, f"{color}.ico")
                    if os.path.exists(ic_path):
                        item.setIcon(QIcon(ic_path))
                    self.lst_users.addItem(item)
            elif kind == "ERR": 
                self.txt_chat.append(f"<div style='color:red; font-family:{self.font_family}; font-size:{self.font_size}pt;'>ERROR: {val}</div>")
            count += 1
            
    def append_chat_msg(self, raw_msg):
        if self.img_buffer is not None:
            prefix = f"{self.img_sender_name}: "
            chunk_to_add = raw_msg
            
            if raw_msg.startswith(prefix):
                chunk_to_add = raw_msg[len(prefix):]
            elif raw_msg.startswith(self.img_sender_name + "|"):
                parts = raw_msg.split(": ", 1)
                if len(parts) > 1 and parts[0].startswith(self.img_sender_name):
                    chunk_to_add = parts[1]

            self.img_buffer += chunk_to_add
            if "[/IMG]" in self.img_buffer:
                self.process_finished_image()
            return

        if "[IMG]" in raw_msg:
            sender = "??"
            content = raw_msg
            
            if ": " in raw_msg:
                parts = raw_msg.split(": ", 1)
                sender_info = parts[0]
                content = parts[1]
                
                if "|" in sender_info:
                    sender = sender_info.split("|")[0]
                else:
                    sender = sender_info
            
            self.img_sender_name = sender
            self.img_buffer = content
            
            if "[/IMG]" in self.img_buffer:
                self.process_finished_image()
            return

        if ": " in raw_msg:
            sender_info, msg = raw_msg.split(": ", 1)
            
            if "|" in sender_info:
                name = sender_info.split("|")[0]
            else:
                name = sender_info
                
            col_name = self.col_me if name == self.ent_name.text() else self.col_friend
            html = (f"<div style='font-family:{self.font_family}; font-size:{self.font_size}pt; margin-bottom:2px;'>"
                    f"<span style='color:{col_name}; font-weight:bold;'>{name}:</span> "
                    f"<span style='color:{self.col_text};'>{msg}</span></div>")
            self.txt_chat.append(html)
        else:
            self.txt_chat.append(f"<div style='color:{self.col_text}; font-family:{self.font_family}; font-size:{self.font_size}pt;'>{raw_msg}</div>")

    def process_finished_image(self):
        try:
            start = self.img_buffer.find("[IMG]") + 5
            end = self.img_buffer.find("[/IMG]")
            
            if start != -1 and end != -1:
                base64_img = self.img_buffer[start:end]
                base64_img = base64_img.replace("\n", "").replace("\r", "").replace(" ", "")
                
                img_id = str(uuid.uuid4())
                self.image_map[img_id] = base64_img
                
                name = self.img_sender_name
                col_name = self.col_me if name == self.ent_name.text() else self.col_friend
                
                html = (f"<div style='font-family:{self.font_family}; font-size:{self.font_size}pt; margin-bottom:2px;'>"
                        f"<span style='color:{col_name}; font-weight:bold;'>{name}:</span> "
                        f"<a href='saveimg:{img_id}' style='color:lightgreen; text-decoration:none;'>[Save]</a><br>"
                        f"<img src='data:image/jpeg;base64,{base64_img}'></div>")
                self.txt_chat.append(html)
            else:
                 self.txt_chat.append("<div style='color:red;'>[Error: Incomplete Image Data]</div>")
        except Exception:
            self.txt_chat.append("<div style='color:red;'>[Error displaying image]</div>")
        
        self.img_buffer = None
        self.img_sender_name = ""

    def open_settings(self):
        self.settings_open = True
        self.update_style(True) 
        
        d = SettingsWindow(self)
        d.exec()
        
        self.settings_open = False
        self.check_focus_state()

    def close_app(self):
        lib.kill()
        try:
            keyboard.unhook_all()
        except:
            pass
        self.close()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = ChatWindow()
    win.setObjectName("ChatWindow")
    win.show()
    sys.exit(app.exec())