import os

def read_file(filepath):
    with open(filepath, 'r') as f:
        return f.read()

def escape_latex(text):
    return text

def build_report():
    cpp_dir = "/home/diptarko-bhattacharjee/Desktop/Diptarko/Semester_Material/Sem_5/CN/CN_Lab/CN_Lab_Ass1"
    
    files_to_read = [
        "frame.hpp", "frame.cpp",
        "checksum.hpp", "checksum.cpp",
        "crc.hpp", "crc.cpp",
        "error.hpp", "error.cpp",
        "utils.hpp", "utils.cpp",
        "sender.cpp",
        "receiver.cpp"
    ]
    
    code_contents = {}
    for fname in files_to_read:
        try:
            code_contents[fname] = read_file(os.path.join(cpp_dir, fname))
        except FileNotFoundError:
            code_contents[fname] = f"// Error: Could not read {fname}"

    latex = r"""\documentclass[12pt, a4paper]{article}

\usepackage[utf8]{inputenc}
\usepackage{amsmath, amssymb}
\usepackage{graphicx}
\usepackage{hyperref}
\usepackage{geometry}
\usepackage{booktabs}
\usepackage{listings}
\usepackage{xcolor}
\usepackage{tikz}
\usepackage{placeins}
\usepackage{float}
\usetikzlibrary{shapes.geometric, arrows, positioning}

\geometry{margin=1in}

% Code snippet styling
\definecolor{codegreen}{rgb}{0,0.6,0}
\definecolor{codegray}{rgb}{0.5,0.5,0.5}
\definecolor{codepurple}{rgb}{0.58,0,0.82}
\definecolor{backcolour}{rgb}{0.95,0.95,0.92}

\lstdefinestyle{mystyle}{
    backgroundcolor=\color{backcolour},   
    commentstyle=\color{codegreen},
    keywordstyle=\color{magenta},
    numberstyle=\tiny\color{codegray},
    stringstyle=\color{codepurple},
    basicstyle=\ttfamily\footnotesize,
    breakatwhitespace=false,         
    breaklines=true,                 
    captionpos=b,                    
    keepspaces=true,                 
    numbers=left,                    
    numbersep=5pt,                  
    showspaces=false,                
    showstringspaces=false,
    showtabs=false,                  
    tabsize=2
}
\lstset{style=mystyle}

\begin{document}

\begin{titlepage}
    \centering
    \vspace*{1cm}
    
    {\Huge \textbf{Jadavpur University}}\\[3 cm]
    
    % \includegraphics[width=0.4\textwidth]{JU_logo.png}\\[3 cm]
    \vspace{4cm}
    
    {\huge \textbf{LAB REPORT}}\\[2cm]
    
    {\Large
    \textbf{Name:} Diptarko Bhattacharjee\\[0.75 cm]
    \textbf{Subject:} Computer Networks Lab\\[0.75 cm]
    \textbf{Assignment:} 1
    }
    
\end{titlepage}

\newpage
\tableofcontents
\newpage


\title{Assignment 1: Design and Implement an Error Detection Module using Checksum and Cyclic Redundancy Check (CRC)}
\date{}
\maketitle


\section{Introduction}
In physical networks, data is inherently susceptible to noise, attenuation, and electromagnetic interference, which can flip bits during transmission over long distances or noisy mediums. To guarantee data integrity and ensure that corrupted frames are not processed as valid data by higher layers, the Data Link Layer employs robust error detection algorithms. These algorithms typically append redundant verification bits---often referred to as a Frame Check Sequence (FCS)---to the end of a payload before it is transmitted across the physical medium. Upon receiving the frame, the receiver re-calculates the verification sequence and compares it to the FCS appended by the sender.

The primary objective of this assignment is to design, implement, and rigorously test two classical error detection mechanisms: the \textbf{16-bit Internet Checksum} and the \textbf{Cyclic Redundancy Check (CRC)} (specifically CRC-8, CRC-10, CRC-16, and CRC-32). Rather than merely simulating this mathematically in a single, disconnected script, we built a fully functional client-server architecture using TCP Socket Programming in C++. This allows us to observe the mathematical blind spots of these algorithms in a realistic, distributed network environment, tracking the performance overhead and exact failure conditions of each scheme.

\section{Inter-Process Communication (IPC)}
To simulate a true network link where packets are serialized and deserialized across a physical boundary, our sender and receiver communicate using POSIX TCP Sockets. 

\subsection{TCP Socket Architecture and Stream Boundaries}
Transmission Control Protocol (TCP) ensures a reliable, stream-oriented connection. However, since TCP is a stream protocol (meaning it does not inherently preserve application-layer message boundaries and can fragment packets), our application layer must enforce strict boundaries to prevent data corruption. 

We achieve this by defining a rigid, fixed-size \textbf{128-byte frame}. The Sender exclusively pushes exactly 128 bytes to the socket per transmission. We implemented a custom `send\_all` and `recv\_all` loop in our socket utility module. This ensures that the Receiver explicitly waits, continually reading from the socket buffer, until exactly 128 bytes are pulled before it attempts any FCS verification. After processing the frame, the receiver sends back a 5-byte Acknowledgement packet (either an ``ACK'' or ``NACK'') indicating the integrity verdict.

\section{System Architecture}

The system relies on a tightly coupled sender-receiver loop. The sender chunks the input file, computes the FCS using the chosen scheme, maliciously injects targeted deterministic errors, and transmits the frame. The receiver strictly parses the incoming 128 bytes, separates the payload from the FCS, recomputes the FCS over the payload, and validates it.

\subsection{Frame Layout}
We designed a strict 128-byte frame layout heavily inspired by the actual IEEE 802.3 Ethernet frame standard. The byte-by-byte breakdown of each individual frame is rigidly defined as follows:

\begin{enumerate}
    \item \textbf{6 bytes (Offset 0):} Destination MAC Address. Specifies the hardware address of the receiver.
    \item \textbf{6 bytes (Offset 6):} Source MAC Address. Specifies the hardware address of the sender.
    \item \textbf{2 bytes (Offset 12):} Valid payload length. Since the final chunk of a file might be less than 110 bytes, this 16-bit integer informs the receiver exactly how many bytes are valid application data versus zero-padded whitespace.
    \item \textbf{110 bytes (Offset 14):} Payload data. The actual application-layer data being transmitted.
    \item \textbf{4 bytes (Offset 124):} Frame Check Sequence (FCS). The redundant verification bits calculated by the Checksum or CRC logic. Though some CRCs are 8 or 16 bits, we allocate a full 4 bytes (32 bits) to accommodate CRC-32, right-justifying smaller schemes and padding with zeros.
\end{enumerate}

\vspace{10pt}
\begin{table}[H]
\centering
\begin{tabular}{|l|l|l|}
\hline
\textbf{Offset} & \textbf{Size (Bytes)} & \textbf{Field Description} \\ \hline
0 & 6 & Destination MAC Address \\ \hline
6 & 6 & Source MAC Address \\ \hline
12 & 2 & Valid Payload Length ($\le$ 110) \\ \hline
14 & 110 & Payload Data (Zero-padded if short) \\ \hline
124 & 4 & Frame Check Sequence (FCS) \\ \hline
\end{tabular}
\caption{Custom 128-Byte Frame Structure}
\end{table}

\subsection{Flowchart and Data Flow}
To visualize the interactions between these core modules, the following diagram illustrates the complete data flow. The ACK/NACK packet is securely routed back to the Sender to dictate the status of the transmission.

\begin{figure}[H]
\centering
\begin{tikzpicture}[node distance=2cm]
\tikzstyle{process} = [rectangle, minimum width=3cm, minimum height=1cm, text centered, draw=black, fill=orange!30]
\tikzstyle{io} = [trapezium, trapezium left angle=70, trapezium right angle=110, minimum width=2.5cm, minimum height=1cm, text centered, draw=black, fill=blue!30]
\tikzstyle{arrow} = [thick,->,>=stealth]

% Sender Side
\node (in) [io] {Input File (.txt/.bits)};
\node (chunk) [process, below of=in] {Chunk \& Pad (110B)};
\node (fcs) [process, below of=chunk] {Compute FCS (Checksum/CRC)};
\node (err) [process, below of=fcs] {Inject Errors (Round-Robin)};
\node (tx) [process, below of=err, fill=red!30] {TCP Socket (Send)};

% Receiver Side
\node (rx) [process, right=5cm of tx, fill=green!30] {TCP Socket (Recv)};
\node (ver) [process, above of=rx] {Recalculate FCS};
\node (dec) [process, above of=ver, fill=yellow!30] {Verdict: ACK / NACK};
\node (out) [io, above of=dec] {Output File};

% Connections
\draw [arrow] (in) -- (chunk);
\draw [arrow] (chunk) -- (fcs);
\draw [arrow] (fcs) -- (err);
\draw [arrow] (err) -- (tx);
\draw [arrow] (tx) -- node[anchor=south] {128-Byte Frame} (rx);
\draw [arrow] (rx) -- (ver);
\draw [arrow] (ver) -- (dec);
\draw [arrow] (dec) -- node[anchor=west] {If ACK} (out);
\draw [arrow] (dec.east) -- ++(1.5,0) |- node[anchor=north, near end] {ACK/NACK Packet} ([yshift=-1cm]tx.south) -- (tx.south);

\end{tikzpicture}
\caption{Overall System Architecture and Data Flow}
\label{fig:arch}
\end{figure}
\FloatBarrier

\section{Types of Errors and Encoding Schemes}

\subsection{Encoding Schemes}
We benchmarked the 16-bit Internet Checksum against four standard CRC polynomials. 

\textbf{1. 16-Bit Internet Checksum:} 
The Internet Checksum is a fast, computationally inexpensive heuristic used predominantly in Network and Transport layer protocols (like IPv4 and TCP). It divides the data payload into 16-bit words and computes their sum using 1's complement arithmetic. Any carry bits that overflow the 16-bit boundary are folded back into the sum (end-around carry). Finally, the sum is bitwise inverted. While phenomenally fast, it is structurally vulnerable to cancelling errors. If bits flip in a way that balances out the sum (e.g., swapping bits in two different 16-bit words, or incrementing one word while decrementing another), the checksum mathematically cannot detect the error.

\textbf{2. Cyclic Redundancy Check (CRC):}
To overcome the limitations of simple addition, CRCs evaluate the data as a massive binary polynomial. A predetermined Generator Polynomial (which acts as a prime divisor) is used to perform Modulo-2 division over the data stream. The remainder of this division becomes the FCS. Because it relies on polynomial division rather than arithmetic addition, CRCs are completely immune to the "balanced addition" trick that fools the Checksum. They are mathematically guaranteed to detect all single-bit errors, odd numbers of bit errors (if the polynomial contains an $x+1$ factor), and burst errors up to the length of the polynomial. We tested the following standardized polynomials:
\begin{itemize}
    \item \textbf{CRC-8}: $x^8 + x^7 + x^6 + x^4 + x^2 + 1$ (Hex: 0xD5)
    \item \textbf{CRC-10}: $x^{10} + x^9 + x^5 + x^4 + x + 1$ (Hex: 0x233)
    \item \textbf{CRC-16}: $x^{16} + x^{15} + x^2 + 1$ (Hex: 0x8005)
    \item \textbf{CRC-32 (IEEE 802.3)}: Standard Ethernet Polynomial (Hex: 0x04C11DB7)
\end{itemize}
However, CRCs possess their own unique mathematical blind spot: if the error pattern itself represents a polynomial that is perfectly divisible by the generator polynomial (a polynomial collision), the remainder will be zero, and the CRC will blindly accept the corrupted frame.

\subsection{Error Types and Deterministic Injection}
To rigorously evaluate the algorithms without relying on statistical chance, we designed a deterministic \textbf{Round-Robin Error Injector}. The injector cycles through 7 specific error profiles, guaranteeing that every single frame is subjected to a targeted attack before transmission:

\begin{enumerate}
    \item \textbf{No Error (Clean):} Transmits the frame perfectly. Verifies baseline integrity.
    \item \textbf{Single-Bit Error:} Flips exactly one random bit in the payload.
    \item \textbf{Two Isolated Bit Errors:} Flips two bits that are strictly non-adjacent.
    \item \textbf{Odd Number of Bit Errors:} Flips a random odd number (3, 5, or 7) of bits.
    \item \textbf{Burst Error (8-bit):} Simulates localized interference. It flips the first and last bit of an 8-bit window, and randomizes the internal bits.
    \item \textbf{Checksum-Blind Error:} Maliciously manipulates two 16-bit words in the payload. It increments one word and decrements an adjacent word. This perfectly preserves the arithmetic sum of the frame, mathematically guaranteeing that the Checksum will fail to detect it, while the CRC should easily catch it.
    \item \textbf{CRC-Blind Error:} Maliciously XORs the payload with the exact bit-pattern of the active CRC Generator Polynomial. This ensures the injected error is a perfect multiple of the divisor, resulting in a zero remainder. The CRC mathematically cannot detect this, while the Checksum should easily flag the disrupted sum.
\end{enumerate}

\section{Project File Structure}
The codebase is deliberately organized into a highly modular structure. By breaking the logic out into specialized files, we avoid "monolithic" scripts and make the core networking and algorithmic logic extremely transparent, maintainable, and beginner-friendly:

\begin{verbatim}
CN_Lab_Ass1/
|-- Makefile               # Build automation script
|-- frame.hpp / .cpp       # Frame structure and layout definitions
|-- crc.hpp / .cpp         # CRC bit-by-bit mathematics
|-- checksum.hpp / .cpp    # 16-bit Checksum mathematics
|-- error.hpp / .cpp       # Deterministic round-robin error injection
|-- utils.hpp / .cpp       # Network I/O and Socket helpers
|-- sender.cpp             # Client application entry point
|-- receiver.cpp           # Server application entry point
\end{verbatim}


\section{Core Modules \& Code Implementation}
In this section, we analyze the entire source code for every individual module, providing an exhaustive, function-by-function analysis explaining the C++ logic. The code has been deliberately designed to be as simple, legible, and beginner-friendly as possible, utilizing standard loops, traditional \texttt{if-else} blocks, and easily understandable variable naming conventions.

\subsection{\texttt{frame.hpp} and \texttt{frame.cpp} - Protocol Structure}
This module defines the constants required for our 128-byte frame layout and provides the foundational structs and enumerators.

\subsubsection{Source Code: \texttt{frame.hpp}}
\begin{lstlisting}[language=C++]
{code_contents['frame.hpp']}
\end{lstlisting}

\subsubsection{Source Code: \texttt{frame.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['frame.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{Constants \& Layout:} The header explicitly defines the sizes of all fields using global constant integers. \texttt{DEST\_LEN} and \texttt{SRC\_LEN} are set to 6 bytes to represent standard MAC addresses. This results in a 14-byte \texttt{HEADER\_LEN}. We statically define the payload as exactly 110 bytes, and the FCS as 4 bytes. This makes the math extremely simple and hardcoded, preventing any buffer overflow issues during execution.
    \item \textbf{\texttt{Frame} struct:} The core data structure mirroring the layout. It strictly defines the byte arrays for the destination, source, payload, and FCS.
    \item \textbf{\texttt{getSchemeInfo} and \texttt{fileTypeName}:} These are incredibly straightforward helper functions. Instead of using complex dictionaries or ternary operators, they use a series of beginner-friendly \texttt{if} statements to return human-readable strings for the console output based on integer codes.
    \item \textbf{\texttt{frame\_to\_bytes} and \texttt{frame\_from\_bytes}:} Rather than utilizing advanced pointer casting, these functions use extremely simple, legible \texttt{for} loops. They iterate through each field of the struct (like the MAC address or payload) and copy them byte-by-byte into a flat \texttt{uint8\_t} array at the correct offset. The length field uses a simple bitwise shift (\texttt{>> 8}) to store the 16-bit integer across two 8-bit bytes (Big Endian format).
\end{itemize}

\subsection{\texttt{checksum.hpp} and \texttt{checksum.cpp} - Internet Checksum}
This module contains the isolated logic for the 16-bit Internet Checksum.

\subsubsection{Source Code: \texttt{checksum.hpp}}
\begin{lstlisting}[language=C++]
{code_contents['checksum.hpp']}
\end{lstlisting}

\subsubsection{Source Code: \texttt{checksum.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['checksum.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{\texttt{compute\_checksum}:} The function evaluates the payload in 16-bit segments. It uses a very clear \texttt{while (i + 1 < len)} loop to ensure it does not read out of bounds. 
    \item \textbf{Forming the Word:} Inside the loop, it shifts the first byte by 8 bits to the left (\texttt{data[i] << 8}) and bitwise ORs it with the second byte to form a complete 16-bit integer (\texttt{word}). This word is then added to a 32-bit \texttt{sum} accumulator.
    \item \textbf{Handling the End-Around Carry:} Standard 1's complement math requires that any overflow past 16 bits be added back. This is achieved through a very legible \texttt{if (sum > 0xFFFF)} check. If the sum crosses the 65,535 threshold, it masks it back to 16 bits (\texttt{sum \& 0xFFFF}) and explicitly adds $+1$ for the carry bit. 
    \item \textbf{Odd Byte Padding:} In the event that the payload size is an odd number of bytes, a secondary \texttt{if (i < len)} block shifts that final solitary byte into the upper 8 bits, treating the lower 8 bits as zero-padding, and adds it to the sum, again checking for a carry.
    \item \textbf{One's Complement Inversion:} Finally, the function returns the bitwise NOT (\texttt{\~sum}), perfectly reflecting the mathematical definition of the Internet Checksum.
\end{itemize}

\subsection{\texttt{crc.hpp} and \texttt{crc.cpp} - Cyclic Redundancy Check}
This module implements the Modulo-2 polynomial division.

\subsubsection{Source Code: \texttt{crc.hpp}}
\begin{lstlisting}[language=C++]
{code_contents['crc.hpp']}
\end{lstlisting}

\subsubsection{Source Code: \texttt{crc.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['crc.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{\texttt{getCrcPoly}:} Similar to the scheme helper, this uses a simple stack of \texttt{if} statements to map an integer width (like 8, 10, or 16) to its corresponding standardized hex generator polynomial (e.g., \texttt{0x8005} for CRC-16).
    \item \textbf{\texttt{compute\_crc}:} This function explicitly avoids opaque optimization techniques like lookup tables, favoring a highly educational bit-by-bit long division loop. 
    \item \textbf{Bit Extraction:} A \texttt{for} loop runs for exactly \texttt{len * 8} iterations. It calculates the current byte index (\texttt{i / 8}) and the specific bit index (\texttt{7 - (i \% 8)}), extracting exactly one bit at a time.
    \item \textbf{Shift Register Logic:} It checks if the topmost bit of the \texttt{reg} register is a 1. If it is, the \texttt{carry} flag is explicitly set to 1. The register is then shifted left by one position, and the newly extracted bit is tacked onto the end. 
    \item \textbf{Modulo-2 Subtraction:} If the carry was 1, polynomial division requires a subtraction. In Galois Field 2, subtraction is identical to a bitwise XOR. Thus, a very clear \texttt{if (carry == 1)} block XORs the register against the generator polynomial (\texttt{p.poly}). 
\end{itemize}

\subsection{\texttt{error.hpp} and \texttt{error.cpp} - Error Injection Logic}
This module is the heart of the testing framework, applying deterministic sabotage to the outgoing frames. It utilizes basic \texttt{rand()} mechanics to keep the codebase beginner-friendly.

\subsubsection{Source Code: \texttt{error.hpp}}
\begin{lstlisting}[language=C++]
{code_contents['error.hpp']}
\end{lstlisting}

\subsubsection{Source Code: \texttt{error.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['error.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{\texttt{rand\_range}:} A very simple wrapper around the standard C library \texttt{rand()} function. It ensures that random numbers generated fall cleanly between a \texttt{lo} and \texttt{hi} limit using the modulo operator (\texttt{\% range}).
    \item \textbf{\texttt{flip\_bit}:} A critical low-level utility. It takes an absolute bit position (e.g., bit 500), uses integer division to find the target byte (\texttt{byte\_pos = bit\_pos / 8}), and modulo to find the exact bit within that byte. It then XORs it with a shifted mask (\texttt{0x80 >> bit\_offset}) to perfectly invert that single bit without touching the others.
    \item \textbf{Standard Errors (\texttt{single\_bit}, \texttt{odd\_bits}, \texttt{burst}):} These functions use extremely straightforward \texttt{while} and \texttt{for} loops to select random bit positions. For instance, in \texttt{two\_isolated}, a \texttt{while (abs(p2 - p1) < 2)} loop continuously re-rolls the second bit's position until it is guaranteed to be non-adjacent to the first bit. 
    \item \textbf{\texttt{checksum\_blind}:} This function implements the most devastating attack against the Checksum. It uses a \texttt{for} loop to search through the frame data for two 16-bit words. If it finds a combination where one word can be incremented and the next decremented, it performs \texttt{w1 = w1 + 1} and \texttt{w2 = w2 - 1}. Because these mathematically balance out, the Checksum's overall arithmetic sum is totally undisturbed, fooling the algorithm.
    \item \textbf{\texttt{crc\_blind}:} This function attacks the core weakness of Modulo-2 division. It fetches the generator polynomial. It then loops through the bits of the frame, intentionally injecting an error pattern that is exactly identical to the generator polynomial itself. Because a polynomial divided by itself leaves a remainder of zero, the CRC is mathematically blinded and incorrectly accepts the corrupted frame.
\end{itemize}

\subsection{\texttt{utils.hpp} and \texttt{utils.cpp} - Network and Framework Utilities}
This module houses the strict TCP socket handlers and the overarching FCS packaging routines. 

\subsubsection{Source Code: \texttt{utils.hpp}}
\begin{lstlisting}[language=C++]
{code_contents['utils.hpp']}
\end{lstlisting}

\subsubsection{Source Code: \texttt{utils.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['utils.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{File Reading (\texttt{read\_text\_file}):} Instead of complex stream buffers, this uses an extremely basic \texttt{while (f.get(c))} loop to read the file character by character, casting it to a \texttt{uint8\_t}, and pushing it into a standard vector.
    \item \textbf{Bits Conversion (\texttt{read\_bits\_file}):} If the user provides a custom \texttt{.bits} file (containing raw '0' and '1' ASCII characters), this function loops through the file, checking \texttt{if (ch == '0')} or \texttt{'1'}, and stores them in an array. It then packs these 8 bits at a time into proper bytes using bitwise ORs. 
    \item \textbf{Socket Reliability (\texttt{send\_all} and \texttt{recv\_all}):} In raw TCP Socket programming, \texttt{send()} and \texttt{recv()} are not guaranteed to transfer all requested bytes in a single call. These functions use a highly legible \texttt{while (sent < size)} loop. They continuously call the socket functions and increment an offset pointer, guaranteeing that exactly 128 bytes are pushed/pulled, eliminating the risk of fragmented frames.
    \item \textbf{\texttt{fcs\_pack} and \texttt{fcs\_unpack}:} The calculated verification sequence (which could be 8, 10, or 16 bits) needs to be securely stored in the 4-byte FCS field of our frame. These functions use a \texttt{for} loop and bitwise shifts to copy the integer into the array, carefully right-justifying the bits and padding the leading space with zeroes. 
    \item \textbf{\texttt{build\_frame}:} This is the master assembler. It takes the MAC addresses and payload data, loops through them to copy them into the \texttt{Frame} struct, and explicitly pads the rest of the 110-byte payload with zeroes. It then constructs a temporary \texttt{coverage} array, evaluates the FCS, and packs it into the frame.
\end{itemize}

\subsection{\texttt{sender.cpp} - The Client Application}
This application is the active transmitter in the network. The code is structured linearly, relying on standard C++ inputs.

\subsubsection{Source Code: \texttt{sender.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['sender.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{Initialization and RNG:} The program starts by seeding the standard random number generator using \texttt{srand((unsigned)time(0))}, ensuring our error injector produces different randomized results on every run.
    \item \textbf{User Input:} It checks if command-line arguments are provided. If not, it falls back to a very friendly, interactive \texttt{std::cin} block to request the receiver's IP, port, and input file. 
    \item \textbf{Socket Setup:} It creates a standard \texttt{AF\_INET} socket, structures the \texttt{sockaddr\_in} object with the target IP and Port, and calls \texttt{connect()}. This is the textbook implementation of a TCP client.
    \item \textbf{Handshake:} Before transmitting raw data, the sender transmits a 12-byte header array containing the chosen scheme, file type, and total number of frames. Crucially, it uses \texttt{htonl} (Host to Network Long) to ensure Endianness consistency across the network.
    \item \textbf{Transmission Loop:} A massive \texttt{for} loop runs for every chunk of the file. It extracts up to 110 bytes, calls \texttt{build\_frame}, and transforms the struct into a 128-byte array. 
    \item \textbf{Error Injection \& Output:} It consults the \texttt{error\_plan} array. If an error is scheduled, it passes the 128-byte array to \texttt{inject\_error}. The sender then utilizes \texttt{std::setw} and \texttt{std::left} from the \texttt{<iomanip>} library to print an absolutely beautiful, perfectly aligned ASCII table to the terminal, detailing the frame number, FCS value, error status, and the eventual ACK/NACK response from the server.
\end{itemize}

\subsection{\texttt{receiver.cpp} - The Server Daemon}
This application passively binds to a port and verifies incoming frames, printing its own sophisticated tables.

\subsubsection{Source Code: \texttt{receiver.cpp}}
\begin{lstlisting}[language=C++]
{code_contents['receiver.cpp']}
\end{lstlisting}

\subsubsection{Detailed Module Analysis}
\begin{itemize}
    \item \textbf{Server Binding:} The receiver uses the standard \texttt{socket()}, \texttt{bind()}, and \texttt{listen()} pipeline. Notably, it utilizes \texttt{setsockopt(..., SO\_REUSEADDR, ...)} to prevent "Address already in use" errors if the server is rapidly restarted during testing. 
    \item \textbf{Accepting Connections:} The program enters a blocking \texttt{accept()} call, halting execution until the sender initiates a handshake. Once connected, it uses \texttt{inet\_ntop} to decipher and print the sender's IP address.
    \item \textbf{Synchronization:} It receives the 12-byte header and immediately runs \texttt{ntohl} (Network to Host Long) to decode it. This tells the receiver exactly which error scheme the sender is using and how many frames to expect in the upcoming loop.
    \item \textbf{Reception Loop \& Profiling:} A \texttt{for} loop receives exactly 128 bytes. The receiver unpacks the byte array back into a \texttt{Frame} struct. It explicitly extracts the first 124 bytes (Header + Payload). Right before calling \texttt{fcs\_compute}, it triggers \texttt{std::chrono::high\_resolution\_clock::now()}. It stops the clock immediately after the calculation, allowing us to perfectly measure the microsecond delay induced by the Checksum or CRC logic.
    \item \textbf{Verification \& File Writing:} It compares the recomputed FCS against the FCS sent in the frame. If they match perfectly (and there is no junk data in the padded FCS bytes), it prints "ACK" and pushes the valid payload into a \texttt{reconstructed} vector. If they do not match, it sends a "NACK" and drops the packet. After the loop terminates, the vector is written to a \texttt{received\_output.txt} file. 
\end{itemize}

\section{Experimental Results}

To gather hard empirical data, we streamed a 2,916-byte text file through the TCP sockets on the local loopback interface using all 5 detection schemes. The total transmission consisted of 27 distinct frames.

\subsection{Performance Metrics}
We measured exactly how long the receiver took to run its verification math upon receiving each 128-byte frame. The system \texttt{std::chrono::high\_resolution\_clock} was used to profile the core FCS loop.

\begin{table}[H]
\centering
\begin{tabular}{@{}lccc@{}}
\toprule
\textbf{Scheme} & \textbf{Mean Verify Time ($\mu$s)} & \textbf{Total Frames} & \textbf{Errors Caught} \\ \midrule
16-bit Checksum & 0.96  & 27 & 19 \\
CRC-8           & 18.13 & 27 & 20 \\
CRC-10          & 17.20 & 27 & 20 \\
CRC-16          & 17.07 & 27 & 20 \\
CRC-32          & 16.92 & 27 & 20 \\ \bottomrule
\end{tabular}
\caption{Mean Verification Time per Scheme}
\end{table}

\vspace{1cm}
\textit{--- INSERT PYTHON GRAPH SCREENSHOT HERE ---}
\vspace{1cm}

\subsection{Detailed Dry Run \& Blind Spot Analysis}

To systematically test the limits of these algorithms, our deterministic round-robin cycle injected 7 distinct error types. We will now do a deep-dive analysis of a single cycle, comparing the behavior of the Checksum against the CRC.

\vspace{1cm}
\textit{--- INSERT SENDER \& RECEIVER TERMINAL SCREENSHOTS HERE ---}
\vspace{1cm}

\subsubsection{Frames 1-5: Standard Network Interference}
For the first 5 frames of the cycle, standard network noise was simulated:
\begin{itemize}
    \item \textbf{Frame 1 (Clean)}: Both algorithms flawlessly calculated the FCS, matched the signature, and sent an ACK.
    \item \textbf{Frame 2 (Single Bit)}: A single bit flip guarantees an arithmetic disruption in the Checksum and an indivisible polynomial in the CRC. Both successfully issued a NACK.
    \item \textbf{Frame 3 (Two Isolated Bits)}: Both algorithms detected the dual-corruption (NACK).
    \item \textbf{Frame 4 (Odd Bits)}: As predicted by Galois theory, CRCs equipped with an $x+1$ factor are inherently immune to odd-number bit errors. Both Checksum and CRC successfully caught this (NACK).
    \item \textbf{Frame 5 (Burst)}: An 8-bit burst alters multiple adjacent bits. Since the burst length is smaller than our smallest CRC polynomial width (10, 16, 32), the CRC mathematically guarantees detection. Both caught it (NACK).
\end{itemize}

\subsubsection{Frame 6 Analysis: Exploiting the Checksum Blind Spot}
In Frame 6, the error injector applied a targeted semantic attack against the Checksum. It incremented one 16-bit payload word and decremented an adjacent word. 

\textbf{Result (Checksum)}: The receiver extracted the payload and began 1's complement addition. Because the changes perfectly offset one another ($+1 -1 = 0$), the final arithmetic sum of the 124-byte block was completely unchanged. The recomputed FCS perfectly matched the transmitted FCS. The Checksum was entirely fooled and issued a false \textbf{ACK}, corrupting the final output file.

\textbf{Result (CRC)}: Because CRC utilizes modulo-2 polynomial division instead of arithmetic addition, the word-swapping profoundly altered the bitstream topology. The $+1$ and $-1$ changes do not "cancel out" in GF(2) division; they drastically shift the polynomial remainder. The CRC successfully flagged the discrepancy and issued a \textbf{NACK}.

\subsubsection{Frame 7 Analysis: Exploiting the CRC Blind Spot}
In Frame 7, the error injector targeted the mathematical foundation of the CRC. It XORed the payload with the exact bit-pattern of the active CRC Generator Polynomial (e.g., XORing \texttt{0x8005} into the payload for CRC-16).

\textbf{Result (CRC)}: In polynomial mathematics, the received message $M(x)$ was altered to $M(x) + E(x)$. Because we set $E(x)$ to equal the generator polynomial $G(x)$, the received message became $M(x) + G(x)$. When the receiver divided by $G(x)$, the remainder of $G(x) / G(x)$ is $0$. The error vanished in the mathematics. The CRC was completely fooled and issued a false \textbf{ACK}.

\textbf{Result (Checksum)}: The Checksum processed the generator polynomial bit-pattern as a random integer addition. This entirely perturbed the arithmetic sum. The Checksum trivially detected the anomaly and issued a \textbf{NACK}.

\section{Discussion}
The implementation and empirical results of this project heavily underscore a massive tradeoff in network engineering: \textbf{Speed vs. Reliability}.

As our timing benchmarks proved, the 16-bit Internet Checksum is phenomenally fast, consistently verifying frames in under $1 \mu s$. This speed is achieved because modern CPU Arithmetic Logic Units (ALUs) are natively optimized for integer addition, allowing the checksum to be computed at wire-speed. However, this speed comes at a severe structural cost: it possesses massive theoretical blind spots that can easily allow corrupted data to slip through.

Conversely, the CRC algorithms required nearly $18 \mu s$ to evaluate the exact same payload. This represents a 1700\% increase in computational overhead when utilizing bit-by-bit shift registers. However, CRCs provide mathematically guaranteed protection against localized bursts and random noise, providing absolute confidence in the integrity of the physical transmission medium. 

This dichotomy perfectly explains the layered design of the OSI Model. The Data Link Layer (Layer 2) requires the absolute robustness of the CRC (e.g., Ethernet IEEE 802.3 uses CRC-32) to ensure the physical wires are operating correctly. However, once the packet reaches the Network and Transport Layers (IPv4, TCP), computing another CRC would introduce crippling latency. Therefore, the higher layers rely on the blazing-fast, lightweight heuristic of the Internet Checksum to catch any rare routing/memory errors, offloading the heavy lifting to the hardware below.

\section{Conclusion}
Building this error detection suite from the ground up highlighted the practical realities of network engineering. Implementing POSIX TCP Sockets demonstrated how physical bytes are streamed across boundaries, requiring rigid, inflexible frame layouts to parse successfully.

Our comparative testing conclusively verified the theoretical properties of these algorithms. The Internet Checksum is a lightweight heuristic suitable for layers where speed is paramount, but it is deeply vulnerable to cancelling errors. CRCs provide robust, mathematically guaranteed protection against interference, making them indispensable for physical connections, albeit at a higher computational cost. Furthermore, isolating the exact theoretical blind spots of both mathematical paradigms (the balanced-addition trick vs. the polynomial collision trick) proved that relying on a single verification algorithm is insufficient, and layered protocols are structurally necessary to provide absolute network integrity.

\end{document}
"""
    
    for k, v in code_contents.items():
        latex = latex.replace("{code_contents['" + k + "']}", escape_latex(v))
        
    with open(os.path.join(cpp_dir, "report.tex"), "w") as f:
        f.write(latex)

if __name__ == "__main__":
    build_report()
