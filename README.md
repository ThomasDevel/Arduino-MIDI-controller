# Arduino-MIDI-controller
A MIDI controller implemented on Arduino


Thanks to the Arduino MIDI library we can generate midi signals with a certain NOTE, VELOCITY and CHANNEL.
These MIDI signals are transport over a MIDI cable. Our PC did not come with a MIDI port (although there
are MIDI adapters available).
We use a virutal MIDI port, for example LoopMIDI.
De MIDI signals that Arduino sends, go through the Arduino Hairless MIDI software to de virtual MIDI port.
A music software program (I am fan of Ableton) recognizes the virtual MIDI port and is able to process
the signals.
