// script.js

// --- Get DOM Elements --- 
const appVersionDisplay = document.getElementById('appVersionDisplay');
// const launchGameBtn = document.getElementById('launchGameBtn'); // Old button
const gameListContainer = document.getElementById('gameListContainer'); 
const statusFooter = document.querySelector('.footer'); 

// New Elements
const libraryTab = document.getElementById('libraryTab');
const storeTab = document.getElementById('storeTab');
const settingsBtn = document.getElementById('settingsBtn');
const gameDetailsContainer = document.getElementById('gameDetailsContainer');
const actionButton = document.getElementById('actionButton');
const progressBarContainer = document.getElementById('progressBarContainer');
const installProgress = document.getElementById('installProgress');
const progressText = document.getElementById('progressText');
const settingsModal = document.getElementById('settingsModal');
const closeSettingsModalBtn = document.getElementById('closeSettingsModal');
const defaultInstallPathInput = document.getElementById('defaultInstallPathInput');
const saveSettingsButton = document.getElementById('saveSettingsButton');

// Auth elements
const loginForm = document.getElementById('loginForm');
const usernameInput = document.getElementById('usernameInput');
const passwordInput = document.getElementById('passwordInput');
const loginButton = document.getElementById('loginButton'); // Might need this if we want to disable during request
const userStatusArea = document.getElementById('userStatusArea');
const loggedInUser = document.getElementById('loggedInUser');
const logoutButton = document.getElementById('logoutButton');

let selectedGame = null; // Store the full selected game object

// Ensure the API namespace exists
window.gameLauncherAPI = window.gameLauncherAPI || {};

// Function called by C++ to update game status in the UI
window.gameLauncherAPI.onStatusUpdate = function(statusUpdate) {
    console.log('Received game status update:', statusUpdate);

    // Update game list entry if it exists
    const gameEntry = document.querySelector(`.game-entry[data-game-id="${statusUpdate.game_id}"]`);
    if (gameEntry) {
        // You could add visual indicators to the list entry here based on status
        // e.g., gameEntry.dataset.status = statusUpdate.status;
        // Maybe update a small status icon within the entry
    }

    // If the updated game is the currently selected one, update the main panel
    if (selectedGame && selectedGame.id === statusUpdate.game_id) {
        // Update the selected game object in memory
        selectedGame.status = statusUpdate.status;
        selectedGame.progress = statusUpdate.progress; // Store progress
        selectedGame.message = statusUpdate.message; // Store message

        // Update the main panel details (if details are showing this game)
        updateGameDetails(selectedGame); // Re-render details, including button state
        updateActionButton(selectedGame); // Update button text and state
        updateProgressBar(selectedGame); // Update progress bar visibility and value
    }
};

// --- Check if CEF communication bridge exists ---
if (typeof window.cefQuery !== 'function') {
    console.error("CEF Query function (window.cefQuery) is not available. IPC bridge failed.");
    if (appVersionDisplay) appVersionDisplay.textContent = 'Bridge Unavailable';
    if (statusFooter) statusFooter.textContent += ' | Communication Bridge: FAILED';
    // Disable functionality that relies on C++ interaction
    if (actionButton) actionButton.disabled = true;
} else {
    if (statusFooter) statusFooter.textContent += ' | Communication Bridge: OK';
    console.log("CEF Query function found. IPC bridge is available.");
}

// --- Function to get the application version ---
function getVersion() {
    console.log("Requesting application version...");
    if (appVersionDisplay) appVersionDisplay.textContent = 'Fetching...';

    window.cefQuery({
        request: 'getVersionRequest:', // Note the colon, even with no payload
        onSuccess: function(response) {
            console.log("Version received:", response);
            if (appVersionDisplay) appVersionDisplay.textContent = response;
            if (statusFooter) {
                // Update only the version part if already appended status
                const baseText = "GameLauncher Status | Version: ";
                if(statusFooter.textContent.includes('| Communication Bridge:')) {
                    statusFooter.textContent = baseText + response + ' | Communication Bridge: OK';
                } else {
                    statusFooter.textContent = baseText + response;
                }
            }
        },
        onFailure: function(errorCode, errorMessage) {
            console.error("Failed to get version. Error code:", errorCode, "Message:", errorMessage);
            if (appVersionDisplay) appVersionDisplay.textContent = 'Error';
            if (statusFooter) statusFooter.textContent = `Error getting version: ${errorMessage}`;
        }
    });
}

// --- Game List Handling ---
/**
 * Requests the game list from the C++ backend and populates the UI.
 */
function fetchAndDisplayGames() {
    console.log('Attempting to fetch game list...');
    if (!gameListContainer) {
        console.error("Game list container not found.");
        return;
    }
    gameListContainer.innerHTML = '<p>Loading games...</p>'; // Loading indicator

    console.log("Requesting game list from backend...");

    if (typeof cefQuery !== 'function') {
        console.error("cefQuery is not available. Cannot fetch game list.");
        gameListContainer.innerHTML = '<p style="color: red;">Error: Communication bridge not available.</p>';
        return;
    }

    console.log('cefQuery for getGameListRequest sent.');
    cefQuery({
        request: 'getGameListRequest:', // Note the colon, assuming C++ expects it even with no payload
        persistent: false,
        onSuccess: function(response) {
            console.log('Received game list response:', response);
            try {
                const games = JSON.parse(response);
                console.log('Parsed games:', games);
                if (!Array.isArray(games)) {
                    throw new Error("Response is not a valid JSON array.");
                }

                gameListContainer.innerHTML = ''; // Clear loading/previous content

                if (games.length === 0) {
                    gameListContainer.innerHTML = '<p>No games found in Library.</p>';
                    console.log('No games found in response or response was empty.');
                } else {
                    games.forEach(game => {
                        const gameElement = document.createElement('div');
                        gameElement.className = 'game-entry'; // Add a class for styling
                        gameElement.id = `game-${game.id}`;
                        gameElement.setAttribute('data-game-id', game.id); // Store game ID
                        // Basic display: Name 
                        gameElement.textContent = game.name || 'Unnamed Game';

                        // Add click listener for selection
                        gameElement.addEventListener('click', () => {
                            // Deselect previous
                            const previouslySelected = gameListContainer.querySelector('.selected');
                            if (previouslySelected) {
                                previouslySelected.classList.remove('selected');
                            }
                            // Select current
                            gameElement.classList.add('selected');
                            selectedGame = game; // Store the whole game object
                            
                            console.log(`Selected game: ${game.name} (ID: ${game.id}, Status: ${game.status})`);

                            // Update main panel content
                            updateGameDetails(game);
                            updateActionButton(game);
                            updateProgressBar(game);
                        });

                        gameListContainer.appendChild(gameElement);
                    });
                    console.log('Game list populated in UI.');
                }

            } catch (error) {
                console.error("Failed to parse or display game list:", error);
                gameListContainer.innerHTML = `<p style="color: red;">Error loading game list: ${error.message}</p>`;
            }
        },
        onFailure: function(errorCode, errorMessage) {
            console.error('Failed to fetch game list:', errorCode, errorMessage);
            gameListContainer.innerHTML = `<p style="color: red;">Error loading game list: ${errorMessage}</p>`;
        }
    });
}

// --- UI Update Functions ---

/**
 * Updates the main panel with details of the selected game.
 * @param {object} game The selected game object.
 */
function updateGameDetails(game) {
    if (!gameDetailsContainer) return;
    if (!game) {
        gameDetailsContainer.innerHTML = '<h1>Select a game</h1><p>Details about the selected game will appear here.</p>';
        return;
    }

    // Basic implementation - replace with richer content later
    gameDetailsContainer.innerHTML = `
        <h1>${game.name || 'Unnamed Game'}</h1>
        <p>Status: ${game.status || 'Unknown'}</p>
        ${game.message ? `<p>Message: ${game.message}</p>` : ''}
        <!-- Add more details like description, image, etc. here -->
    `;
}

/**
 * Sends a request to the backend to perform an action (install, update, cancel).
 * @param {string} gameId The ID of the game.
 * @param {string} actionType 'install', 'update', or 'cancel'.
 */
function requestGameAction(gameId, actionType) {
    console.log(`Requesting action '${actionType}' for game ID: ${gameId}`);
    if (!gameId) {
        console.error("No game ID provided for action request.");
        return;
    }

    const payload = { gameId: gameId, action: actionType };

    cefQuery({
        request: `gameActionRequest:${JSON.stringify(payload)}`,
        persistent: false,
        onSuccess: function(response) {
            console.log(`Action '${actionType}' request successful for game ${gameId}. Response:`, response);
            // The UI should update via the async onStatusUpdate callback
        },
        onFailure: function(errorCode, errorMessage) {
            console.error(`Failed to request action '${actionType}' for game ${gameId}. Error ${errorCode}: ${errorMessage}`);
            // Optionally update UI to show error immediately
            if (selectedGame && selectedGame.id === gameId) {
                 selectedGame.status = 'ERROR'; // Update local state
                 selectedGame.message = `Action failed: ${errorMessage}`;
                 updateActionButton(selectedGame);
                 updateProgressBar(selectedGame);
            }
        }
    });
}

/**
 * Sends a request to the backend to launch a game.
 * @param {string} gameId The ID of the game to launch.
 */
function requestLaunch(gameId) {
    console.log(`Requesting launch for game ID: ${gameId}`);
    if (!gameId) {
        console.error("No game ID provided for launch request.");
        return;
    }

    const payload = { gameId: gameId };

    cefQuery({
        request: `requestLaunch:${JSON.stringify(payload)}`,
        persistent: false,
        onSuccess: function(response) {
            console.log(`Launch request successful for game ${gameId}. Response:`, response);
            // UI updates to 'LAUNCHING' state via the async onStatusUpdate callback triggered by C++
        },
        onFailure: function(errorCode, errorMessage) {
            console.error(`Failed to request launch for game ${gameId}. Error ${errorCode}: ${errorMessage}`);
            // Update UI to show error immediately
             if (selectedGame && selectedGame.id === gameId) {
                 selectedGame.status = 'ERROR'; // Update local state
                 selectedGame.message = `Launch failed: ${errorMessage}`;
                 updateGameDetails(selectedGame); // Show error message in details
                 updateActionButton(selectedGame);
                 updateProgressBar(selectedGame);
            }
        }
    });
}

/**
 * Updates the state (text, enabled/disabled, listener) of the main action button
 * based on the selected game's status.
 * @param {object} game The selected game object.
 */
function updateActionButton(game) {
    if (!actionButton) return;

    // Remove existing listeners to prevent duplicates
    actionButton.replaceWith(actionButton.cloneNode(true));
    // Re-fetch the button after cloning
    const newActionButton = document.getElementById('actionButton'); 
    if (!newActionButton) return; // Should not happen, but safety check
    const currentActionButton = newActionButton; // Use the new button reference

    if (!game) {
        currentActionButton.textContent = 'Action';
        currentActionButton.disabled = true;
        return;
    }

    currentActionButton.disabled = false; // Enable by default, disable specific cases

    switch (game.status) {
        case 'NOT_INSTALLED':
            currentActionButton.textContent = 'Install';
            currentActionButton.onclick = () => requestGameAction(game.id, 'install');
            break;
        case 'INSTALLING':
        case 'UPDATING':
            currentActionButton.textContent = game.status === 'INSTALLING' ? 'Installing...' : 'Updating...';
            currentActionButton.disabled = false; // Allow cancel
            currentActionButton.onclick = () => requestGameAction(game.id, 'cancel');
            // Optionally change text to 'Cancel'
            // currentActionButton.textContent = 'Cancel'; 
            break;
        case 'CANCELLED':
            currentActionButton.textContent = 'Install'; // Or 'Update' if previously installed
            currentActionButton.onclick = () => requestGameAction(game.id, 'install'); // Or 'update'
            break;
        case 'INSTALLED':
            currentActionButton.textContent = 'Launch';
            currentActionButton.disabled = false;
            currentActionButton.onclick = () => requestLaunch(game.id); // Call the launch function
            break;
        case 'LAUNCHING': // New state
            currentActionButton.textContent = 'Launching...';
            currentActionButton.disabled = true; // Disable while launching
            currentActionButton.onclick = null; // Remove listener
            break;
        case 'RUNNING': // Placeholder for future
            currentActionButton.textContent = 'Running';
            currentActionButton.disabled = true; // Can't do much while running
            currentActionButton.onclick = null;
            break;
        case 'ERROR':
            currentActionButton.textContent = 'Retry Install'; // Or more specific based on error context
            currentActionButton.disabled = false; // Allow retry
            currentActionButton.onclick = () => requestGameAction(game.id, 'install'); // Or 'update'
            break;
        default:
            currentActionButton.textContent = 'Unknown State';
            currentActionButton.disabled = true;
            currentActionButton.onclick = null;
            console.warn(`Unhandled game status for action button: ${game.status}`);
    }
}

/**
 * Updates the visibility and value of the progress bar.
 * @param {object} game The selected game object.
 */
function updateProgressBar(game) {
    if (!progressBarContainer || !installProgress || !progressText) return;

    if (game && (game.status === 'INSTALLING' || game.status === 'UPDATING')) {
        progressBarContainer.style.display = 'block'; // Show progress bar
        const progress = game.progress || 0;
        installProgress.value = progress;
        progressText.textContent = `${Math.round(progress)}%`;
    } else {
        progressBarContainer.style.display = 'none'; // Hide progress bar for other states
        installProgress.value = 0;
        progressText.textContent = '0%';
    }
}

// --- Function to request an action on a game (Launch, Install, Update, Cancel) ---
function performGameAction(game) {
    if (!game) {
        console.error('performGameAction called without a selected game.');
        return;
    }
    console.log(`Requesting action for game ID: ${game.id}, Status: ${game.status}`);

    let actionType = '';
    switch (game.status) {
        case 'ReadyToPlay':       actionType = 'launch'; break;
        case 'NotInstalled':      actionType = 'install'; break;
        case 'UpdateRequired':    actionType = 'update'; break;
        case 'Installing':
        case 'Updating':
        case 'Downloading':       actionType = 'cancel'; break;
        default:
            console.error(`No valid action for game status: ${game.status}`);
            alert(`Cannot perform action for game status: ${game.status}`);
            return;
    }

    const payload = JSON.stringify({ game_id: game.id, action: actionType });
    const requestString = `gameActionRequest:${payload}`;

    // Optional: Provide immediate UI feedback
    if (actionType === 'launch') {
        updateActionButton({ ...game, status: 'Launching' }); // Optimistic UI update
    } else if (actionType === 'install' || actionType === 'update') {
        updateProgressBar({ ...game, status: actionType === 'install' ? 'Installing' : 'Updating', progress: 0});
    }
    // We expect an onStatusUpdate message from C++ to confirm/update state

    alert(`Sending ${actionType} request for game ID: ${game.id}`); // Simple feedback

    window.cefQuery({
        request: requestString,
        onSuccess: function(response) {
            console.log(`${actionType} request successful:`, response);
            // Status should ideally be updated via onStatusUpdate from C++
            // alert(`${actionType} request for ${game.name} acknowledged.`); 
        },
        onFailure: function(errorCode, errorMessage) {
            console.error(`Failed to perform ${actionType} action. Error:`, errorCode, errorMessage);
            alert(`Failed to ${actionType} game ${game.name}. Error: ${errorMessage}`);
            // Revert optimistic UI updates if necessary
            updateGameDetails(game); 
            updateActionButton(game);
            updateProgressBar(game);
        }
    });
}

// --- Authentication Functions ---

/**
 * Sends a login request to the backend.
 * @param {string} username 
 * @param {string} password 
 */
function loginUser(username, password) {
    console.log(`Attempting login for user: ${username}`);
    // TODO: Optionally disable login button here

    const payload = JSON.stringify({ username: username, password: password });

    window.cefQuery({
        request: 'loginRequest:' + payload,
        onSuccess: function(response) {
            console.log('Login successful:', response);
            try {
                const result = JSON.parse(response);
                if (result.status === 'success') {
                    // TODO: Fetch user profile details if needed, for now just use username
                    updateUIForLoggedIn(username);
                } else {
                    // Handle potential backend-specific failure messages if status wasn't 'success'
                    console.error('Login failed on backend:', result);
                    alert('Login failed. Please check your credentials.'); // Simple feedback
                }
            } catch (e) {
                console.error('Error parsing login response:', e, 'Raw response:', response);
                alert('Login failed due to unexpected response.');
            }
            // TODO: Re-enable login button if disabled
        },
        onFailure: function(errorCode, errorMessage) {
            console.error('Login request failed:', errorCode, errorMessage);
            alert(`Login failed: ${errorMessage}`); // Show error from backend
            // TODO: Re-enable login button if disabled
        }
    });
}

/**
 * Sends a logout request to the backend.
 */
function logoutUser() {
    console.log('Attempting logout...');
    // TODO: Optionally disable logout button here

    window.cefQuery({
        request: 'logoutRequest:',
        onSuccess: function(response) {
            console.log('Logout successful:', response);
             try {
                const result = JSON.parse(response);
                if (result.status === 'success') {
                   updateUIForLoggedOut();
                } else {
                    console.error('Logout failed on backend:', result);
                    alert('Logout failed.'); 
                }
            } catch (e) {
                console.error('Error parsing logout response:', e, 'Raw response:', response);
                alert('Logout failed due to unexpected response.');
            }
            // TODO: Re-enable logout button if disabled
        },
        onFailure: function(errorCode, errorMessage) {
            console.error('Logout request failed:', errorCode, errorMessage);
            alert(`Logout failed: ${errorMessage}`);
            // TODO: Re-enable logout button if disabled
        }
    });
}

/**
 * Updates the UI elements to reflect a logged-in state.
 * @param {string} username The username to display.
 */
function updateUIForLoggedIn(username) {
    if (loginForm) loginForm.style.display = 'none';
    if (userStatusArea) userStatusArea.style.display = 'flex'; // Or 'block', depending on CSS
    if (loggedInUser) loggedInUser.textContent = username;
}

/**
 * Updates the UI elements to reflect a logged-out state.
 */
function updateUIForLoggedOut() {
    if (loginForm) loginForm.style.display = 'flex'; // Or 'block', depending on CSS
    if (userStatusArea) userStatusArea.style.display = 'none';
    if (loggedInUser) loggedInUser.textContent = '';
    // Clear form fields
    if (usernameInput) usernameInput.value = '';
    if (passwordInput) passwordInput.value = '';
}

// --- Initial Auth Check ---
/**
 * Checks the initial authentication status with the backend when the page loads.
 */
function checkInitialAuthStatus() {
    console.log('Checking initial authentication status...');
    window.cefQuery({
        request: 'getAuthStatusRequest:',
        onSuccess: function(response) {
            console.log('Initial auth status response:', response);
            try {
                const result = JSON.parse(response);
                if (result.status === 'loggedIn' && result.profile && result.profile.username) {
                    console.log('User is logged in:', result.profile.username);
                    updateUIForLoggedIn(result.profile.username);
                } else {
                    console.log('User is logged out.');
                    updateUIForLoggedOut();
                }
            } catch (e) {
                console.error('Error parsing initial auth status response:', e, 'Raw response:', response);
                updateUIForLoggedOut(); // Default to logged out on error
            }
        },
        onFailure: function(errorCode, errorMessage) {
            console.error('Failed to get initial auth status:', errorCode, errorMessage);
            updateUIForLoggedOut(); // Default to logged out on failure
        }
    });
}

// --- Initial Actions ---
getVersion(); // Get the version when the page loads
fetchAndDisplayGames(); // Fetch the game list when the page loads
checkInitialAuthStatus(); // Check if user is already logged in
updateGameDetails(null); // Show initial placeholder message
updateActionButton(null); // Disable button initially
updateProgressBar(null); // Hide progress bar initially

// --- Application Settings Functions ---

/**
 * Fetches the application settings from the backend.
 * @returns {Promise<object>} A promise that resolves with the settings object 
 *                            or rejects with an error message.
 */
function getAppSettings() {
    console.log('Requesting application settings...');
    return new Promise((resolve, reject) => {
        window.cefQuery({
            request: 'getAppSettingsRequest',
            persistent: false,
            onSuccess: function(response) {
                try {
                    const settings = JSON.parse(response);
                    console.log('Received settings:', settings);
                    resolve(settings);
                } catch (e) {
                    console.error('Error parsing settings JSON:', e, 'Raw response:', response);
                    reject('Failed to parse settings from backend.');
                }
            },
            onFailure: function(errorCode, errorMessage) {
                console.error(`Error getting settings: ${errorCode} - ${errorMessage}`);
                reject(`Error getting settings: ${errorMessage} (Code: ${errorCode})`);
            }
        });
    });
}

/**
 * Sends updated application settings to the backend.
 * @param {object} settingsObject - The settings object to save.
 * @returns {Promise<string>} A promise that resolves with a success message 
 *                           or rejects with an error message.
 */
function setAppSettings(settingsObject) {
    console.log('Sending updated application settings:', settingsObject);
    return new Promise((resolve, reject) => {
        if (typeof settingsObject !== 'object' || settingsObject === null) {
            return reject('Invalid settings object provided.');
        }
        try {
            const settingsJsonString = JSON.stringify(settingsObject);
            window.cefQuery({
                request: `setAppSettingsRequest:${settingsJsonString}`,
                persistent: false,
                onSuccess: function(response) {
                    console.log('Settings update successful:', response);
                    resolve(response); // Usually a simple success message
                },
                onFailure: function(errorCode, errorMessage) {
                    console.error(`Error setting settings: ${errorCode} - ${errorMessage}`);
                    reject(`Error setting settings: ${errorMessage} (Code: ${errorCode})`);
                }
            });
        } catch (e) {
            console.error('Error stringifying settings object:', e);
            reject('Failed to serialize settings object before sending.');
        }
    });
}

// Example usage (you would call these from your UI logic, e.g., in response to button clicks)
/*
getAppSettings()
    .then(settings => {
        console.log('Successfully fetched settings:', settings);
        // Update UI elements with fetched settings
        // Example: document.getElementById('install-path-input').value = settings.default_install_path;
    })
    .catch(error => {
        console.error('Failed to fetch settings:', error);
        // Display error to the user
    });

// Example of setting settings (e.g., after user clicks save)
document.getElementById('save-settings-button')?.addEventListener('click', () => {
    const newSettings = {
        default_install_path: document.getElementById('install-path-input').value
        // Add other settings from UI elements
    };
    setAppSettings(newSettings)
        .then(message => {
            console.log(message);
            // Show success message to user
        })
        .catch(error => {
            console.error('Failed to save settings:', error);
            // Show error message to user
        });
});
*/

// --- Settings Modal Logic ---

// Function to open the settings modal and load current settings
function openSettingsModal() {
    console.log('Opening settings modal...');
    getAppSettings()
        .then(settings => {
            console.log('Populating settings modal with:', settings);
            if (settings && typeof settings.default_install_path !== 'undefined') {
               defaultInstallPathInput.value = settings.default_install_path;
            } else {
                console.warn('Could not find default_install_path in settings response.');
                defaultInstallPathInput.value = ''; // Clear if not found
            }
             // Populate other settings fields here if added
            settingsModal.style.display = 'block'; // Show the modal
        })
        .catch(error => {
            console.error('Failed to load settings for modal:', error);
            // Optionally show an error to the user before opening
            // or open with default/empty values
            defaultInstallPathInput.value = ''; 
            settingsModal.style.display = 'block'; // Still show modal, maybe with an error message inside?
            // TODO: Add user-facing error message display within the modal
            alert(`Error loading settings: ${error}`); // Simple alert for now
        });
}

// Function to close the settings modal
function closeSettingsModal() {
    settingsModal.style.display = 'none';
}

// Function to save settings
function saveSettings() {
    const newSettings = {
        default_install_path: defaultInstallPathInput.value.trim()
        // Collect other settings values here
    };
    console.log('Attempting to save settings:', newSettings);
    // Optional: Add validation here

    setAppSettings(newSettings)
        .then(message => {
            console.log('Settings saved successfully:', message);
            alert('Settings saved successfully!'); // Simple confirmation
            closeSettingsModal();
        })
        .catch(error => {
            console.error('Failed to save settings:', error);
            // TODO: Display error message within the modal instead of alert
            alert(`Error saving settings: ${error}`);
        });
}

// --- Event Listeners ---

// Settings Button Click
if (settingsBtn) {
    settingsBtn.addEventListener('click', openSettingsModal);
} else {
    console.error('Settings button (#settingsBtn) not found.');
}

// Settings Modal Close Button Click
if (closeSettingsModalBtn) {
    closeSettingsModalBtn.addEventListener('click', closeSettingsModal);
} else {
    console.error('Settings modal close button (#closeSettingsModal) not found.');
}

// Save Settings Button Click
if (saveSettingsButton) {
    saveSettingsButton.addEventListener('click', saveSettings);
} else {
    console.error('Save settings button (#saveSettingsButton) not found.');
}

// Action Button (Launch/Install/Update/Cancel)
if (actionButton) {
    actionButton.addEventListener('click', () => {
        if (!selectedGame) {
             console.warn('Action button clicked, but no game is selected.');
             alert('Please select a game first!');
             return;
         }
        performGameAction(selectedGame);
    });
}

// Add listener for login form submission
if (loginForm) {
    loginForm.addEventListener('submit', (event) => {
        event.preventDefault(); // Prevent default form submission
        const username = usernameInput.value.trim();
        const password = passwordInput.value; // Don't trim password
        if (username && password) {
            loginUser(username, password);
        } else {
            alert('Please enter both username and password.');
        }
    });
}

// Add listener for logout button click
if (logoutButton) {
    logoutButton.addEventListener('click', () => {
        logoutUser();
    });
}

// Tab Buttons (Placeholders)
if (libraryTab) {
    libraryTab.addEventListener('click', () => {
        console.log('Library tab clicked');
        // TODO: Implement logic to show Library view (which is default for now)
        storeTab.classList.remove('active');
        libraryTab.classList.add('active');
        // Potentially hide/show different main panel content sections
        fetchAndDisplayGames(); // Refresh library view
        updateGameDetails(null); // Clear details when switching tabs
        updateActionButton(null);
        updateProgressBar(null);
    });
}
if (storeTab) {
    storeTab.addEventListener('click', () => {
        console.log('Store tab clicked');
        // TODO: Implement logic to show Store view
        libraryTab.classList.remove('active');
        storeTab.classList.add('active');
        gameListContainer.innerHTML = '<p>Store coming soon!</p>'; // Placeholder
        gameDetailsContainer.innerHTML = '<h1>Store</h1><p>Browse games here.</p>'; // Placeholder
        updateActionButton(null); // No action button in store view for now
        updateProgressBar(null);
        selectedGame = null; // Deselect game when switching to store
        // Deselect item in list visually
        const previouslySelected = gameListContainer.querySelector('.selected');
        if (previouslySelected) { 
            previouslySelected.classList.remove('selected');
        }
    });
}

// Settings Button (Placeholder)
if (settingsBtn) {
    settingsBtn.addEventListener('click', () => {
        console.log('Settings button clicked');
        alert('Settings panel not yet implemented.');
        // TODO: Implement logic to show settings panel (e.g., modal or separate view)
    });
}

// TODO: Consider adding a check to C++ on startup to see if already logged in
// and update the UI accordingly.
