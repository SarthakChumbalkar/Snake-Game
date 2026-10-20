# Music Player Search System

A C++ console-based Music Player Search System developed as a Data Structures and Algorithms (DSA) micro project. The program loads song details from a text file, searches songs and lyrics, sorts the library, searches for exact titles, and plays a user-selected queue through the Windows default media player.

## Features

- Display all songs with their IDs, titles, artists, albums, and release years.
- Search songs by title, artist, or album.
- Search lyrics using a keyword.
- Play an individual song using the Windows default application associated with the audio file.
- Create a playback queue by entering song IDs.
- Move to the next queue entry after the current media-player process closes.
- Sort songs alphabetically by title using Bubble Sort.
- Search for an exact title using Binary Search.
- Normalize search text by ignoring case, spaces, and punctuation.

## Technologies Used

- **Language:** C++
- **Standard:** C++17
- **Data structures:** Array and singly linked list
- **Algorithms:** Linear Search, Bubble Sort, Binary Search
- **File handling:** C++ file streams (`ifstream`)
- **Windows integration:** `ShellExecuteExA` to open audio files with the default media application
- **Build tools:** MinGW-w64 / GCC (for example, MSYS2 UCRT64)

## Project Structure

```text
Music-Player/
├── main.cpp
├── README.md
├── songs.txt
├── music/
│   ├── alag_aasmaan.mp3
│   ├── arz_kiya_hai.mp3
│   └── ...
└── lyrics/
    ├── alag_aasmaan.txt
    ├── arz_kiya_hai.txt
    └── ...
```

### File descriptions

| File or folder | Purpose |
|---|---|
| `main.cpp` | Contains the menu, searching and sorting algorithms, linked-list queue, and playback functions. |
| `songs.txt` | Stores the metadata for each song. |
| `music/` | Contains the audio files. |
| `lyrics/` | Contains lyrics text files used by the lyrics-search feature. |

## Song Database Format

Each line in `songs.txt` represents one song. Fields are separated by the pipe character (`|`) in this order:

```text
ID|Title|Artist|Album|Year|AudioFilename|LyricsFilename
```

Example:

```text
1|Alag Aasmaan|Anuv Jain|Alag Aasmaan - Single|2020|alag_aasmaan.mp3|alag_aasmaan.txt
2|Arz Kiya Hai|Anuv Jain|Coke Studio Bharat Season 3|2025|arz_kiya_hai.mp3|arz_kiya_hai.txt
3|Badtameez Dil|Benny Dayal, Shefali Alvares|Yeh Jawaani Hai Deewani|2013|badtameez_dil.mp3|badtameez_dil.txt
```

Make sure that each audio filename matches a file in `music/` and each lyrics filename matches a file in `lyrics/`. The program expects relative paths from the project folder and supports up to 100 song records.

## Requirements

- Windows
- A C++ compiler with C++17 support, such as MinGW-w64 GCC
- An audio player associated with your audio file type in Windows
- Audio files and metadata configured as described above

This version uses the Windows Shell to open each song in the default media application. It does not require FFmpeg, FFplay, or the Windows MCI API.

## How to Compile and Run

Open PowerShell in your project folder. For example:

```powershell
cd D:\Learning\DSA\Music-Player
```

Compile the program:

```powershell
g++ main.cpp -std=c++17 -o temp_music.exe -lshell32
```

Run it if compilation succeeds:

```powershell
if ($LASTEXITCODE -eq 0) { .\temp_music.exe }
```

The `-std=c++17` flag enables C++17 features, including `std::filesystem`. The `-lshell32` flag links the Windows Shell library used by `ShellExecuteExA`.

## Main Menu

```text
===== MUSIC PLAYER SEARCH SYSTEM =====
1. Display All Songs
2. Search by Song Title
3. Search by Artist
4. Search by Album
5. Search by Lyrics Keyword
6. Play a Song
7. Create Queue and Play Songs
8. Sort Songs by Title
9. Binary Search by Exact Title
10. Exit
```

## How Queue Playback Works

1. Select **7. Create Queue and Play Songs**.
2. The program displays the library and asks for song IDs separated by spaces.
3. Enter IDs in the order you want to play them. For example:

   ```text
   2 1 9 8
   ```

4. The program builds a singly linked list in that order.
5. Each audio file is opened using the Windows default media player.
6. When Windows provides a handle to the launched player process, the program waits for that process to close before opening the next queue item.

### Important playback note

The Windows default media application controls audio playback. Depending on the application, Windows may not provide a process handle—for example, when the file is handed to an already-running or single-instance app. In that case, the program asks you to press Enter in the console after closing the media player so it can continue to the next song. Queue advancement therefore depends on how the configured media application is launched by Windows.

This implementation does not provide in-program Previous, Pause, Resume, or Stop controls. Use the media player's own controls while a track is open.

## Data Structures

### Array

The library is stored in a fixed-size array:

```cpp
Song songs[MAX_SONGS];
```

The array allows the program to iterate over song records for display, searching, and sorting. `MAX_SONGS` is set to 100.

### Singly Linked List

The playback queue uses nodes containing a song and a pointer to the next node:

```cpp
struct QueueNode {
    Song song;
    QueueNode* next;
};
```

- `front` points to the first queued song.
- `rear` points to the last queued song.
- `enqueue()` adds a song at the rear.
- `dequeue()` removes a song from the front when clearing the queue.
- `clearQueue()` releases the allocated nodes after playback or before building a new queue.

A singly linked list is suitable for this version because queue playback proceeds forward only.

## Algorithms

### Linear Search

Used for title, artist, album, and lyrics-keyword searches. The program checks each song until it finds matches.

### Bubble Sort

Used to arrange songs by normalized title. It compares adjacent titles and swaps them when they are out of order.

### Binary Search

Used to find a song by exact normalized title. The function first copies and sorts the library, then performs Binary Search on the sorted copy.

### Text Normalization

The `normalize()` function converts letters to lowercase and removes characters that are not alphanumeric. As a result, these inputs normalize to the same value:

| Input | Normalized text |
|---|---|
| `Alag Aasmaan` | `alagaasmaan` |
| `ALAG-AASMAAN!` | `alagaasmaan` |

Normalization ignores capitalization, spacing, and punctuation; it does not remove or translate different letters. For example, `Phir Se Ud Chala` and `PHIR-SE-UD-CHALA` match after normalization.

## Time Complexity

Let **n** be the number of songs in the library.

| Operation | Time complexity |
|---|---:|
| Display all songs | O(n) |
| Linear search by title, artist, or album | O(n) comparisons (excluding string-comparison costs) |
| Lyrics search | Depends on the total size of the lyrics files read; each listed lyrics file is checked |
| Enqueue one song | O(1) |
| Dequeue one song | O(1) |
| Clear a queue with q nodes | O(q) |
| Bubble Sort | O(n²) |
| Binary Search after the function sorts a copy | O(n²) overall, dominated by Bubble Sort |
| Binary Search on already sorted data | O(log n) comparisons |

## Limitations

- The program is Windows-specific because it uses `ShellExecuteExA` and Windows headers.
- The library is limited to 100 song records.
- The `songs.txt` file must follow the specified pipe-separated format.
- Audio and lyrics files must be placed in the expected folders.
- Playback advancement depends on how the Windows default media app launches and whether Windows returns a process handle.
- If Windows does not provide a process handle, manual confirmation with Enter is required before the queue advances.
- Binary Search only matches an exact normalized title; title and artist linear searches also support partial matches.
- Queue entries are not saved as a playlist file.

## Possible Future Improvements

- Add a reliable playback process or library that reports when a track actually finishes.
- Add pause, resume, stop, and previous-track functionality.
- Save and load playlists.
- Add song metadata editing and removal.
- Add a graphical user interface, album artwork, and shuffle/repeat modes.
- Replace the fixed-size song array with a dynamic container if a larger library is needed.

## Learning Outcomes

This project demonstrates:

- C++ structures, arrays, pointers, and dynamic memory allocation.
- Singly linked lists and queue operations.
- File handling with text files.
- Linear Search, Bubble Sort, and Binary Search.
- String normalization and keyword matching.
- Basic Windows integration for opening files in the default application.

## Conclusion

The Music Player Search System applies core DSA concepts in a practical C++ console project. It supports song and lyrics searches, sorting, exact-title lookup, and forward queue playback through the Windows default media application.

**Project type:** DSA Micro Project  
**Language:** C++  
**Platform:** Windows  
**Build standard:** C++17
