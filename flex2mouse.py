from time import sleep
from playsound import playsound
from itertools import cycle
import websocket

import autopy

# Okay, great. Next step is to start making this into a looping client that
# does what I want.

MIN_SWITCH       = 2500   # EMG readings above this switch mouse mode.
MAX_MOVE         = 2000   # EMG readings above this either deadzone or switch.
MIN_MOVE         =   50   # EMG readings below this won't trigger movement.
MOVE_SENSITIVITY =  0.2   # 1 means +1 readin = +1 pixel moved.
MIN_CLICK        =   50   # Minimum signal in to trigger a click.
CHIME = True

TICKS_TO_QUIT = 3 # How long to stay in mouse_quit mode before we exit.

INSTANT_MOVE = False  # Change whether the mouse moves smoothly or not.
                      # Usually not a good idea to turn on, but if your
                      # sensitivity is cranked up to the point smooth_move
                      # is throwing off your timing, it might be worth a shot.


ip = "ws://192.168.0.210"

mouse_mvmts_possible = ['mouse_right', 'mouse_down', 'mouse_left', 'mouse_up', \
                            'mouse_left_click', 'mouse_right_click', 'exit']

mouse_mvmts = cycle(mouse_mvmts_possible)



def pixel_move(mag):
    if (mag > MAX_MOVE):
        return 0
    elif (mag < MIN_MOVE):
        return 0
    else:
        return (mag - MIN_MOVE) * MOVE_SENSITIVITY


def next_mouse_mvmt():
    return next(mouse_mvmts)

def next_location(mvmt, mag):
    if (mvmt == "mouse_right"):
        return [autopy.mouse.location()[0] + mag, autopy.mouse.location()[1]]
    elif (mvmt == "mouse_down"):
        return [autopy.mouse.location()[0], autopy.mouse.location()[1] + mag] # Weird! Down is +, and up is -. Usually think of it the other way.
    elif (mvmt == "mouse_left"):
        return [autopy.mouse.location()[0] - mag, autopy.mouse.location()[1]]
    elif (mvmt == "mouse_up"):
        return [autopy.mouse.location()[0], autopy.mouse.location()[1] - mag]
    else:
        return autopy.mouse.location()

def bounded_next_location(mvmt, mag):
    nl = next_location(mvmt, mag)

    if (nl[0] < 0.0):
        nl[0] = 0.0
    elif (nl[0] > autopy.screen.size()[0] - 10.0):
        nl[0] = autopy.screen.size()[0] - 10.0
    if (nl[1] < 0.0):
        nl[1] = 0.0
    elif (nl[1] > autopy.screen.size()[1] - 10.0):
        nl[1] = autopy.screen.size()[1] - 10.0

    return nl


if __name__=="__main__":
    print("Connecting to the ESP-32.")
    ws = websocket.WebSocket()
    ws.connect(ip)
    print("Complete.")

    print("Entering movement loop.")

    quit_ticks_left = TICKS_TO_QUIT

    mouse_mvmt = next_mouse_mvmt()
    print("Mouse movement this cycle: {}".format(mouse_mvmt))
    c = 500
    while (c > 0):
        c = c - 1
        print("--------------------------")

        raw_in = int(ws.recv())
        print("Raw muscle sensor reading: {}".format(raw_in))

        if (raw_in > MIN_SWITCH):
            if (CHIME):
                playsound('./sounds/tick_switch-cursor.wav')
            print("\n===>    NEXT_MOVEMENT   <===\n")

            mouse_mvmt = next_mouse_mvmt()

            print("Mouse movement changed: {}".format(mouse_mvmt))
        elif (MIN_SWITCH > raw_in > MAX_MOVE):
            if (CHIME):
                playsound('./sounds/tick_dead-zone.wav')
            print("\n===>        DEAD_ZONE   <===\n")

            print("Values between {} and {} produce nothing.".format(MAX_MOVE, MIN_SWITCH))
        else:
            if (CHIME):
                playsound('./sounds/tick_move.wav')
            print("\n===>   MOUSE_MOVE       <===\n")

            if (mouse_mvmt == 'exit'):
                print("Mouse `movement`: {}".format(mouse_mvmt))
                print("Stay here for {} more ticks to quit the program.\n\n\n".format(quit_ticks_left))
                if (quit_ticks_left == 0):
                    break
                else:
                    quit_ticks_left -= 1
            else:
                quit_ticks_left = TICKS_TO_QUIT

            if (mouse_mvmt == 'mouse_left_click'):
                if (raw_in > MIN_CLICK):
                    print("Performing click: {}.".format(mouse_mvmt))
                    print("Pressing down ...")
                    autopy.mouse.toggle(autopy.mouse.Button.LEFT, True)
                    print("... Releasing ... ")
                    autopy.mouse.toggle(autopy.mouse.Button.LEFT, False) # Simulate a button click, arbitrarily fast.
                    print("... Done.")
                else:
                    print("Muscle didn't flex enough to perform click.")
            elif (mouse_mvmt == 'mouse_right_click'):
                if (raw_in > MIN_CLICK):
                    print("Performing click: {}.".format(mouse_mvmt))
                    print("Pressing down ...")
                    autopy.mouse.toggle(autopy.mouse.Button.RIGHT, True)
                    print("... Releasing ... ")
                    autopy.mouse.toggle(autopy.mouse.Button.RIGHT, False) # Simulate a button click, arbitrarily fast.
                    print("... Done.")
                else:
                    print("Muscle didn't flex enough to perform click.")
            else:
                pixels = pixel_move(raw_in)
                if (pixels == 0):
                    print("Muscles not tensed enough; we will not move.\n")
                else:
                    nl = bounded_next_location(mouse_mvmt, pixels)
                    print("Moving {} pixels in direction {}.".format(pixels, mouse_mvmt))
                    print("Next location will be: {}.".format(nl))
                    if (INSTANT_MOVE):
                        autopy.mouse.move(nl[0], nl[1])
                    else:
                        autopy.mouse.smooth_move(nl[0], nl[1])
                    pass # put in code here.
                print("Now at {}.".format(autopy.mouse.location()))
    # Gracefully close WebSocket connection
    print("Closing out the connection. Thank you!")
    ws.close()
