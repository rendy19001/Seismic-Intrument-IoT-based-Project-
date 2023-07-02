import curses
import sys
import serial
import time
import threading


title = "Geofon IoT"
menu = ['Start', 'Help']
version = "Version 1.0.0"
developer= "by rendy"


def read_serial(ser, serial_win):
    fcnt = 0
    ch = ''
    global data
    while True:
        
        sline = ''
        while ch != b'\n':
            ch = ser.read()
            if ch !=b'\n':
                sline +=ch.decode()

        read = sline.strip()
        serial_win.addstr(read + '\n')
        
        ch = ''

        if sline == 'json':
            sline =''
            lvl = 0

            while True:
                ch = ser.read()

                if ch == b'{':
                    lvl+=1
                sline+=ch.decode()

                if ch == b'}':
                    lvl -=1

                    if lvl == 0:
                        fname="%06d.json"%(fcnt)
                        with open(fname, 'w') as fl:
                            fl.write(sline + '\n')
                        fcnt+=1
                        sline=''
                        serial_win.clear()
                        serial_win.addstr("== json save to local ==")
                        break
        
                
        serial_win.refresh()


def print_menu(stdscr, selected_row_idx):
    stdscr.clear()
    h, w = stdscr.getmaxyx()

    for idx, row in enumerate (menu):
        x = w//2 - len(row)//2
        y = h//2 - len(menu)//2 + idx
        if idx == selected_row_idx:
            stdscr.attron(curses.color_pair(1))
            stdscr.addstr(y, x, row)
            stdscr.attroff(curses.color_pair(1))
        else:
            stdscr.addstr(y, x, row)
    
    stdscr.refresh()

def print_title(stdscr):
    h, w = stdscr.getmaxyx()
    x = w//2 - len(title)//2
    y = h//3

    stdscr.addstr(y+2, x, "==========")
    stdscr.addstr(y, x, title)
    stdscr.addstr(y-2, x, "==========")

def print_version(stdscr):
    h, w = stdscr.getmaxyx()
    x = w - 4 - len(version)
    y = h - 4

    stdscr.addstr(y, x, version)

def print_developer(stdscr):
    h, w = stdscr.getmaxyx()
    x = w - 4 - len(developer)
    y = h - 3

    stdscr.addstr(y, x, developer)

def init_serial(port, baudrate):
    try:
        ser = serial.Serial(port, baudrate)
        ser.flushInput()
        ser.flushOutput()
        return ser
    except serial.SerialException as e:
        print("Error opening serial port: " + str(e))
        exit(1)


def main(stdscr):
    curses.curs_set(0)
    curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE)

    port = 'COM5'  # Ganti dengan port serial yang sesuai
    baudrate = 115200  # Ganti dengan baudrate yang sesuai
    ser = init_serial(port, baudrate)
    h, w = stdscr.getmaxyx()

    # Membuat window untuk judul
    title2 = "Serial Monitor"
    title_win = curses.newwin(3, w, 0, 0)
    title_win.refresh()

    # Buat kotak untuk input perintah
    input_win = curses.newwin(6, w, 3, 0)
    input_win.refresh()

    # Buat kotak untuk menampilkan bacaan serial
    serial_win = curses.newwin(h-9, w, 9, 0)
    serial_win.scrollok(False)  # Mengizinkan scrolling
    serial_win.refresh()

    current_row_idx = 0

    while 1 :

        print_menu(stdscr, current_row_idx)
        print_title(stdscr)
        print_version(stdscr)
        print_developer(stdscr)
    
        
        key = stdscr.getch()
        
        if key == curses.KEY_UP and current_row_idx > 0:
            current_row_idx -= 1
        elif key == curses.KEY_DOWN and current_row_idx < len(menu)- 1:
            current_row_idx += 1
        elif key == curses.KEY_ENTER or key in [10,13]:
            if current_row_idx == 0:
                #stdscr.clear()
                stdscr.refresh()

                #serial_win.clear()

                # Buat thread baru untuk membaca data serial
                serial_thread = threading.Thread(target=read_serial, args=(ser, serial_win))
                serial_thread.daemon = True
                serial_thread.start()

                while 1:

                    title_win.addstr(0, w//2 - len(title)//2, "==========")
                    title_win.addstr(1, w//2 - len(title)//2, title )
                    title_win.addstr(2, w//2 - len(title)//2, "==========")
                    title_win.refresh()

                    serial_win.clear()
                    serial_win.refresh()
                    # Baca input perintah
                    input_win.move(1, 0)
                    input_win.clrtoeol()

                    curses.echo()
                    input_win.addstr(1, 0, "insert command >>   ")
                    input_win.addstr(3, w//2 - len(title2)//2, "==============")
                    input_win.addstr(4, w//2 - len(title2)//2,title2)
                    input_win.addstr(5, w//2 - len(title2)//2, "==============")
                    input_win.addstr(2, w-20, "'clc', clear window  ")
                    input_win.refresh()
                    #cmd = input_win.getstr().decode()

                    cmd_y, cmd_x = 1, len("insert command >>   ")
                    cmd = input_win.getstr(cmd_y, cmd_x, w - len("insert command >>   ")).decode()
                    curses.noecho()


                    # Kirim perintah ke perangkat Arduino
                    ser.write(cmd.encode())

                     # Hentikan program jika tombol 'q' ditekan
                    if cmd == 'q':
                        sys.exit()
                    
                    if cmd == 'clc':
                        serial_win.clear()

            elif current_row_idx == 1:
                stdscr.clear()
                stdscr.refresh()
                stdscr.addstr(0,0,"kamu menekan help")
                stdscr.refresh()
                stdscr.getch()


# Jalankan TUI
curses.wrapper(main)
