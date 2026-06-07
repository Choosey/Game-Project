const API_BASE = 'http://localhost:3000/api';

let gameState = {
  word: '',
  currentRow: 0,
  currentCol: 0,
  gameOver: false,
  won: false,
  guessHistory: []
};

const board = document.getElementById("board");
const keyboard = document.getElementById("keyboard");

async function initGame() {
  try {
    const response = await fetch(`${API_BASE}/game/init`);
    const data = await response.json();
    gameState.word = data.targetWord || 'APPLE';
    gameState.currentRow = 0;
    gameState.currentCol = 0;
    gameState.gameOver = false;
    gameState.won = false;
    gameState.guessHistory = [];
  } catch (error) {
    console.error('Failed to init game:', error);
    gameState.word = 'APPLE';
  }
}

function startGame() {
  document.getElementById("menu").classList.add("hidden");
  document.getElementById("howToPlay").classList.add("hidden");
  document.getElementById("game").classList.remove("hidden");
  initGame();
  createBoard();
  createKeyboard();
}

function goMenu() {
  location.reload();
}

function openHowToPlay() {
  document.getElementById("menu").classList.add("hidden");
  document.getElementById("howToPlay").classList.remove("hidden");
}

async function restartGame() {
  document.body.classList.remove("win", "lost");
  const winText = document.getElementById("winText");
  winText.classList.add("hidden");
  winText.classList.remove("show");
  
  await initGame();
  createBoard();
  createKeyboard();
}

function createBoard() {
  board.innerHTML = "";
  for (let r = 0; r < 6; r++) {
    let row = document.createElement("div");
    row.classList.add("row");

    for (let c = 0; c < 5; c++) {
      let tile = document.createElement("div");
      tile.classList.add("tile");
      row.appendChild(tile);
    }

    board.appendChild(row);
  }
}

function createKeyboard() {
  keyboard.innerHTML = "";

  const layout = [
    ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
    ["A", "S", "D", "F", "G", "H", "J", "K", "L"],
    ["ENTER", "Z", "X", "C", "V", "B", "N", "M", "⌫"]
  ];

  layout.forEach(row => {
    let div = document.createElement("div");
    div.classList.add("keyrow");

    row.forEach(key => {
      let btn = document.createElement("button");
      btn.textContent = key;
      btn.id = `key-${key}`;

      if (key === "ENTER" || key === "⌫") {
        btn.classList.add("big");
      }

      btn.onclick = () => handleKey(key);

      div.appendChild(btn);
    });

    keyboard.appendChild(div);
  });
}

document.addEventListener("keydown", e => {
  let key = e.key.toUpperCase();
  if (key === "BACKSPACE") key = "⌫";
  if (key === "ENTER" || key === "⌫" || /^[A-Z]$/.test(key)) {
    handleKey(key);
  }
});

function handleKey(key) {
  if (gameState.gameOver) return;

  let rows = document.querySelectorAll(".row");
  let tiles = rows[gameState.currentRow].children;

  if (key === "⌫") {
    if (gameState.currentCol > 0) {
      gameState.currentCol--;
      tiles[gameState.currentCol].textContent = "";
    }
    return;
  }

  if (key === "ENTER") {
    if (gameState.currentCol === 5) {
      checkWord(tiles);
    }
    return;
  }

  if (gameState.currentCol < 5) {
    tiles[gameState.currentCol].textContent = key;
    gameState.currentCol++;
  }
}

async function checkWord(tiles) {
  let guess = "";
  for (let t of tiles) guess += t.textContent;

  if (guess.length !== 5) {
    showError("Word must be 5 letters");
    return;
  }

  try {
    const response = await fetch(`${API_BASE}/game/submit`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ guess: guess })
    });

    if (!response.ok) {
      const error = await response.json();
      showError(error.error || "Invalid guess");
      return;
    }

    const result = await response.json();

    // Apply colors to tiles based on backend validation
    if (result.letterResults && result.letterResults.length > 0) {
      for (let i = 0; i < result.letterResults.length; i++) {
        const lr = result.letterResults[i];
        const colorClass = lr.color === 'green' ? 'correct' 
                         : lr.color === 'yellow' ? 'present' 
                         : 'absent';
        tiles[i].classList.add(colorClass);
      }

      // Update keyboard colors
      updateKeyboardColors(result.letterResults);
    }

    gameState.guessHistory.push({
      word: guess,
      results: result.letterResults
    });

    if (result.correct) {
      // Win
      gameState.gameOver = true;
      gameState.won = true;
      document.body.classList.add("win");
      const winText = document.getElementById("winText");
      winText.classList.remove("hidden");
      winText.classList.add("show");
      setTimeout(() => showPopup("YOU WON !! Great Job ! 🎉"), 600);
    } else if (result.gameOver) {
      // Loss
      gameState.gameOver = true;
      document.body.classList.add("lost");
      const stateData = await fetch(`${API_BASE}/game/state`).then(r => r.json());
      setTimeout(() => showPopup(`Game Over! Word was: ${stateData.targetWord}`), 200);
    } else {
      // Game continues
      gameState.currentRow++;
      gameState.currentCol = 0;
    }

  } catch (error) {
    console.error('Error submitting guess:', error);
    showError("Connection error. Check backend.");
  }
}

function updateKeyboardColors(letterResults) {
  for (const lr of letterResults) {
    const btn = document.getElementById(`key-${lr.letter}`);
    if (btn) {
      btn.classList.remove('correct', 'present', 'absent');
      if (lr.color === 'green') {
        btn.classList.add('correct');
      } else if (lr.color === 'yellow') {
        btn.classList.add('present');
      } else if (lr.color === 'grey') {
        btn.classList.add('absent');
      }
    }
  }
}

function showError(message) {
  const errorDiv = document.createElement('div');
  errorDiv.style.cssText = `
    position: fixed;
    top: 20px;
    left: 50%;
    transform: translateX(-50%);
    background: #c9b458;
    color: white;
    padding: 10px 20px;
    border-radius: 5px;
    z-index: 100;
    font-weight: bold;
  `;
  errorDiv.textContent = message;
  document.body.appendChild(errorDiv);
  setTimeout(() => errorDiv.remove(), 2000);
}

function showPopup(message) {
  alert(message);
}
