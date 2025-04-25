import React, { useState, useEffect, useMemo } from 'react';
import './index.css'; // Ensure Tailwind is imported

// --- Component Interfaces ---
interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'Downloading' | 'Installing' | 'UpdateRequired' | 'NotInstalled' | 'Verifying' | 'Repairing'; // Added Verifying/Repairing
  imageUrl: string;
  description: string;
  version: string;
  progress?: number; // Optional download/install progress (0-100)
}

interface UserProfile {
  username: string;
  // Add other profile details: avatarUrl, email, etc.
}

type AuthStatus =
  | { status: 'LoggedIn'; profile: UserProfile }
  | { status: 'LoggedOut' }
  | { status: 'LoggingIn' }
  | { status: 'LoggingOut' }
  | { status: 'Error'; error?: string; profile?: UserProfile | null }; // Keep profile on error if possible

// Define the API structure for type safety (ensure this matches your actual API)
interface GameLauncherAPI {
  getGameList: (onSuccess: (games: Game[]) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  getAuthStatus: (onSuccess: (status: AuthStatus) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  getVersion: (onSuccess: (version: string) => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  performGameAction: (action: 'install' | 'launch' | 'update', gameId: string, onSuccess: () => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  login: (onSuccess: () => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  logout: (onSuccess: () => void, onFailure: (errorCode: number, errorMessage: string) => void) => void;
  // Add other methods as needed
}

// Extend the Window interface to declare the injected API
declare global {
  interface Window {
    gameLauncherAPI?: GameLauncherAPI;
    cefQuery?: (query: { request: string; persistent?: boolean; onSuccess: (response: any) => void; onFailure: (error_code: number, error_message: string) => void }) => void;
    // Optional: Add cefQueryCancel if needed
  }
}

// --- Child Components ---

// Game Item Component
const GameItem: React.FC<{ game: Game; isSelected: boolean; onClick: () => void }> = React.memo(({ game, isSelected, onClick }) => {
  return (
    <div
      className={`flex items-center p-3 rounded-lg mb-2 cursor-pointer transition-colors duration-150 ease-in-out ${isSelected ? 'bg-purple-700 shadow-md' : 'bg-gray-700 hover:bg-gray-600'}`}
      onClick={onClick}
    >
      <img src={game.imageUrl} alt={game.name} className="w-16 h-10 object-cover rounded mr-4" />
      <div className="flex-grow">
        <h3 className={`font-semibold ${isSelected ? 'text-white' : 'text-gray-200'}`}>{game.name}</h3>
        <p className={`text-xs ${isSelected ? 'text-purple-200' : 'text-gray-400'}`}>{game.status}</p>
      </div>
    </div>
  );
});

// Game Details Component
const GameDetails: React.FC<{ game: Game | null; onAction: (gameId: string, action: string) => void }> = ({ game, onAction }) => {
  if (!game) {
    return <div className="text-center text-gray-500 p-10">Select a game to see details</div>;
  }

  const renderActionButton = () => {
    switch (game.status) {
      case 'ReadyToPlay':
        return <button onClick={() => onAction(game.id, 'launch')} className="w-full bg-green-600 hover:bg-green-700 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out">Launch</button>;
      case 'UpdateRequired':
        return <button onClick={() => onAction(game.id, 'update')} className="w-full bg-blue-600 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out">Update</button>;
      case 'NotInstalled':
        return <button onClick={() => onAction(game.id, 'install')} className="w-full bg-purple-600 hover:bg-purple-700 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out">Install</button>;
      case 'Downloading':
      case 'Installing':
      case 'Verifying':
      case 'Repairing':
        return (
          <div className="w-full bg-gray-600 rounded h-8 flex items-center justify-center relative overflow-hidden">
            <div className="absolute left-0 top-0 h-full bg-blue-500 transition-all duration-300 ease-linear" style={{ width: `${game.progress || 0}%` }}></div>
            <span className="relative z-10 text-white text-sm font-medium">{game.status}... {game.progress || 0}%</span>
          </div>
        );
      default:
        return <button className="w-full bg-gray-500 text-white font-bold py-2 px-4 rounded cursor-not-allowed" disabled>Unavailable</button>;
    }
  };

  return (
    <div className="flex flex-col h-full bg-gray-850 rounded-lg overflow-hidden shadow-inner">
      <img src={game.imageUrl} alt={game.name} className="w-full h-48 object-cover" />
      <div className="p-6 flex-grow flex flex-col">
        <h2 className="text-2xl font-bold mb-2 text-white">{game.name}</h2>
        <p className="text-gray-400 text-sm mb-4 flex-grow">{game.description}</p>
        <div className="text-xs text-gray-500 mb-4">Version: {game.version} | Status: {game.status}</div>
        <div className="mt-auto">
          {renderActionButton()}
        </div>
      </div>
    </div>
  );
};

// Authentication Area Component
const AuthArea: React.FC<{ 
  authStatus: AuthStatus | null;
  onLogin: () => void; 
  onLogout: () => void;
  loginError: string | null;
}> = ({ authStatus, onLogin, onLogout, loginError }) => {
  const [isLoading, setIsLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsLoading(true);
    // TODO: In a real scenario, get credentials securely.
    // For now, assume onLogin triggers the appropriate flow.
    try {
      await onLogin(); // Call the parent handler
      // Parent component will update state based on login result
      if (authStatus?.status !== 'Error' && authStatus?.status !== 'LoggingIn') {
         setIsLoading(false);
      }
    } catch (error) {
      // Error should be handled and displayed via loginError prop by parent
      console.error("Error during login submission in AuthArea:", error);
    } finally {
      setIsLoading(false);
    }
  };

  if (authStatus?.status === 'LoggingIn' || authStatus?.status === 'LoggingOut' || isLoading) {
    return <div className="text-center text-gray-400 p-4">Loading...</div>;
  }

  if (authStatus?.status === 'LoggedIn') {
    return (
      <div className="text-center p-4 bg-gray-700 rounded-lg shadow-md">
        <p className="text-green-400 mb-2">Welcome, {authStatus.profile.username}!</p>
        <button onClick={onLogout} className="w-full bg-red-600 hover:bg-red-700 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out">
          Logout
        </button>
      </div>
    );
  }

  // Logged Out or Error state
  return (
    <div className="p-4 bg-gray-700 rounded-lg shadow-md">
      <h3 className="text-lg font-semibold mb-3 text-center text-white">Login</h3>
      <form onSubmit={handleSubmit} className="space-y-3">
        {loginError && (
          <p className="text-red-400 text-sm text-center">{loginError}</p>
        )}
        <button type="submit" disabled={isLoading} className="w-full bg-purple-600 hover:bg-purple-700 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out disabled:opacity-50">
          {isLoading ? 'Logging in...' : 'Login / Authenticate'}
        </button>
      </form>
    </div>
  );
};

// --- Main App Component ---
function App() {
  const [games, setGames] = useState<Game[]>([]);
  const [selectedGameId, setSelectedGameId] = useState<string | null>(null);
  const [isLoadingGames, setIsLoadingGames] = useState(true); // Changed to true initially
  const [authStatus, setAuthStatus] = useState<AuthStatus | null>(null);
  const [version, setVersion] = useState<string>('N/A'); // Default to N/A
  const [loginError, setLoginError] = useState<string | null>(null);
  const [api, setApi] = useState<GameLauncherAPI | null>(null);
  const [apiError, setApiError] = useState<string | null>(null); // State for API connection errors

  // --- API Initialization ---
  useEffect(() => {
    const checkForApi = async () => {
      // Give CEF potentially a bit more time
      await new Promise(resolve => setTimeout(resolve, 1500)); // Wait for potential injection

      if (window.gameLauncherAPI) {
        console.log("Real API found. Setting base API state.");
        const resolvedApi = window.gameLauncherAPI;
        setApi(resolvedApi); // Set the base API, triggers apiWrapper creation
      } else {
        console.error("CRITICAL: window.gameLauncherAPI not found after delay!");
        setApiError("Failed to connect to the launcher backend. Please restart the launcher.");
        // Keep loading games false so the error message shows
        setAuthStatus({ status: 'Error', error: 'API unavailable' });
        setVersion('N/A');
        setGames([]);
      }

      // Only set loading to false after attempting to fetch or determining API is missing
      setIsLoadingGames(false);
    };

    checkForApi();

    // No cleanup needed for API listener in this version

  }, []); // Empty dependency array ensures this runs once on mount

  // Wrapper for the injected API to handle response parsing
  const apiWrapper: GameLauncherAPI | undefined = useMemo(() => {
    if (!api) return undefined;

    console.log("Real API found. Wrapping it to parse responses.");

    // Define the wrapped API methods
    return {
      getGameList: (onSuccess, onFailure) => {
        window.cefQuery?.({
          request: 'getGameListRequest:{}',
          onSuccess: (responseString) => { // CEF success gives a string
            try {
              const parsedResponse = JSON.parse(responseString); // Parse it
              onSuccess(parsedResponse); // Call component's callback with parsed data
            } catch (e) {
              console.error("Failed to parse getGameList response:", e, "Raw:", responseString);
              onFailure(999, "Failed to parse response");
            }
          },
          onFailure: onFailure, // Pass failure callback directly
        });
      },
      getAuthStatus: (onSuccess, onFailure) => {
        window.cefQuery?.({
          request: 'getAuthStatusRequest:{}',
          onSuccess: (responseString) => {
            try {
              const parsedResponse = JSON.parse(responseString);
              onSuccess(parsedResponse);
            } catch (e) {
              console.error("Failed to parse getAuthStatus response:", e, "Raw:", responseString);
              onFailure(999, "Failed to parse response");
            }
          },
          onFailure: onFailure,
        });
      },
      getVersion: (onSuccess, onFailure) => {
         window.cefQuery?.({
          request: 'getVersionRequest:{}',
          onSuccess: (responseString) => {
            try {
              const parsedResponse = JSON.parse(responseString);
              // Assuming the response is like { version: "x.y.z" }
              if (typeof parsedResponse === 'object' && parsedResponse !== null && 'version' in parsedResponse) {
                onSuccess(parsedResponse.version); // Pass the actual version string
              } else {
                 console.error("Failed to parse version from response:", parsedResponse);
                 onFailure(998, "Invalid version format");
              }
            } catch (e) {
              console.error("Failed to parse getVersion response:", e, "Raw:", responseString);
              onFailure(999, "Failed to parse response");
            }
          },
          onFailure: onFailure,
        });
      },
       performGameAction: (action, gameId, onSuccess, onFailure) => {
         window.cefQuery?.({
          request: `performGameActionRequest:${JSON.stringify({ action, gameId })}`, // Include payload
          onSuccess: onSuccess, // Assume success is just confirmation (no data)
          onFailure: onFailure,
        });
      },
       login: (onSuccess, onFailure) => {
         window.cefQuery?.({
          request: 'loginRequest:{}', // No payload needed usually
          onSuccess: onSuccess, // TODO: Check if login returns profile data that needs parsing
          onFailure: onFailure,
        });
      },
       logout: (onSuccess, onFailure) => {
         window.cefQuery?.({
          request: 'logoutRequest:{}',
          onSuccess: onSuccess, // Assume success is confirmation
          onFailure: onFailure,
        });
      },
    };
  }, [api]); // Re-create wrapper if the underlying api object changes (e.g., on injection)

  // --- Initial Data Fetching (uses the wrapper) ---
  useEffect(() => {
    // Only run this effect if the apiWrapper is ready
    if (apiWrapper) {
      console.log("API Wrapper ready. Fetching initial data...");
      setIsLoadingGames(true); // Set loading true before fetch

      try {
        // Fetch Game List using the wrapper
        apiWrapper.getGameList(
          (fetchedGames) => {
             if (Array.isArray(fetchedGames)) {
               setGames(fetchedGames);
               // Select first game if list isn't empty and none is selected
               if (fetchedGames.length > 0 && !selectedGameId) {
                 setSelectedGameId(fetchedGames[0].id);
               }
             } else {
               console.error('[Initial Fetch] ERROR: Fetched game data is not an array!', fetchedGames);
               setApiError('Received invalid game data format from backend.');
               setGames([]); // Reset to empty array on error
             }
          },
          (errCode, errMsg) => {
            console.error("Failed to fetch initial game list:", errCode, errMsg);
            setApiError(`Failed to load games: ${errMsg} (${errCode})`);
            setGames([]); // Ensure games is empty on error
          }
        );

        // Fetch Auth Status using the wrapper
        apiWrapper.getAuthStatus(
          (status) => {
            console.log("[Initial Fetch] Auth Status Success:", status);
            setAuthStatus(status);
          },
          (errCode, errMsg) => {
            console.error("Failed to fetch initial auth status:", errCode, errMsg);
            setAuthStatus({ status: 'Error', error: `Failed to get auth status: ${errMsg}` });
          }
        );

        // Fetch Version using the wrapper
        apiWrapper.getVersion(
          (v) => {
            console.log("[Initial Fetch] Version Success:", v);
            setVersion(v);
          },
          (errCode, errMsg) => {
            console.error("Failed to fetch version:", errCode, errMsg);
            setVersion('Error');
          }
        );

      } catch (error: any) {
        console.error("[Initial Fetch] Synchronous error during initial data fetch:", error);
        setApiError('An unexpected error occurred while fetching initial data.');
        setGames([]);
        setAuthStatus({ status: 'Error', error: 'API Fetch Error' });
        setVersion('Error');
      } finally {
        // Set loading false after all initial fetches are attempted
        setIsLoadingGames(false);
      }
    } else if (api === null && !isLoadingGames) {
       // This handles the case where the initial API check finished and found nothing
       // We only set loading false here if it wasn't already set by the apiWrapper block
       setIsLoadingGames(false);
    }

  }, [apiWrapper]); // This effect runs when apiWrapper becomes available/changes

  // Handler for login action
  const handleLogin = () => { 
    if (!api) {
       setLoginError("Login service unavailable.");
       return;
    }
    setLoginError(null); // Clear previous errors
    setAuthStatus({ status: 'LoggingIn' }); // Set loading state
    console.log("Attempting login via API...");
    try {
      // Call the API's login method (no user/pass here, assumes external flow or stored token)
      api.login(
        () => {
          console.log("Login successful (callback)");
          // Refresh auth status after successful login
          api.getAuthStatus(
            (fetchedStatus) => setAuthStatus(fetchedStatus),
            (authErrorCode, authErrorMsg) => {
              console.error("Failed to refresh auth status after login:", authErrorCode, authErrorMsg);
              setAuthStatus({ status: 'Error', error: `Login successful, but failed to refresh status: ${authErrorMsg}` });
            }
          );
        },
        (errorCode, errorMessage) => {
          console.error("Login failed (callback):", errorCode, errorMessage);
          const errorText = errorMessage || 'Login failed.';
          setAuthStatus({ status: 'Error', error: errorText }); // Use errorMessage string
          setLoginError(errorText); // Use errorMessage string
        }
      );
    } catch (error: any) { // Catch potential synchronous errors
        console.error("Synchronous error during login call:", error);
        const errorMsg = error.message || 'An unexpected error occurred during login.';
        setAuthStatus({ status: 'Error', error: errorMsg });
        setLoginError(errorMsg);
    }
  };

  // Handler for logout action
  const handleLogout = () => {
    if (!api) {
      setLoginError("Logout service unavailable."); // Use loginError state for consistency
      return;
    }
    setLoginError(null); // Clear previous errors
    setAuthStatus({ status: 'LoggingOut' }); // Set loading state
    console.log("Attempting logout via API...");
    try {
        api.logout(
            () => {
                console.log("Logout successful (callback)");
                // Refresh auth status after successful logout
                api.getAuthStatus(
                    (fetchedStatus) => setAuthStatus(fetchedStatus),
                    (authErrorCode, authErrorMsg) => {
                      console.error("Failed to refresh auth status after logout:", authErrorCode, authErrorMsg);
                      setAuthStatus({ status: 'Error', error: `Logout successful, but failed to refresh status: ${authErrorMsg}` });
                    }
                  );
            },
            (errorCode, errorMessage) => {
                console.error("Logout failed (callback):", errorCode, errorMessage);
                const errorText = errorMessage || 'Logout failed.';
                setAuthStatus({ status: 'Error', error: errorText }); // Use errorMessage string
                setLoginError(errorText); // Use errorMessage string
            }
        );
    } catch (error: any) { // Catch potential synchronous errors
        console.error("Synchronous error during logout call:", error);
        const errorMsg = error.message || 'An unexpected error occurred during logout.';
        setAuthStatus({ status: 'Error', error: errorMsg });
        setLoginError(errorMsg);
    }
  };

  // Handler for game actions (install, launch, update)
  const handleGameAction = (gameId: string, actionString: string) => {
    const validActions = ['install', 'launch', 'update'];
    if (!validActions.includes(actionString)) {
        console.error(`Invalid action string received: ${actionString}`);
        alert(`Invalid action: ${actionString}`);
        return;
    }
    const action = actionString as 'install' | 'launch' | 'update';

    if (!api) {
      alert("Action service unavailable.");
      return;
    } 

    console.log(`Performing action: ${action} for game: ${gameId}`);
    try {
        api.performGameAction(action, gameId,
            () => {
                console.log(`${action} action successful (callback) for ${gameId}`);
                const gameName = games.find(g => g.id === gameId)?.name || gameId; 
                alert(`${action} initiated for ${gameName}`); 

                // Example: Refresh game list after action
                // api.getGameList(
                //     (fetchedGames) => setGames(fetchedGames),
                //     (error) => console.error('Error refreshing game list after action:', error)
                // );
            },
            (error) => {
                console.error(`${action} action failed (callback) for ${gameId}:`, error);
                const gameName = games.find(g => g.id === gameId)?.name || gameId; 
                alert(`Failed to ${action} ${gameName}: ${error}`); 
            }
        );
    } catch (error: any) { 
        console.error(`Synchronous error during ${action} call for ${gameId}:`, error);
        const gameName = games.find(g => g.id === gameId)?.name || gameId; 
        alert(`An unexpected error occurred while trying to ${action} ${gameName}.`);
    }
  };

  const selectedGame = games.find(game => game.id === selectedGameId) || null;

  // --- Render Logic ---

  if (isLoadingGames) {
    return (
      <div className="flex items-center justify-center h-screen bg-gray-900 text-white text-lg">
        Initializing Launcher...
      </div>
    );
  }

  if (apiError) {
    return (
      <div className="flex flex-col items-center justify-center h-screen bg-red-900 text-white p-8 text-center">
        <h2 className="text-2xl font-bold mb-4">Connection Error</h2>
        <p className="mb-6">{apiError}</p>
        {/* Optionally add a retry button? */}
      </div>
    );
  }

  return (
    <div className="flex flex-col h-screen bg-gradient-to-br from-gray-900 to-gray-800 text-gray-100 font-sans overflow-hidden">
      {/* Header Area */}
      <header className="bg-gray-950 shadow-lg p-3 flex justify-between items-center flex-shrink-0 border-b border-gray-700">
        <h1 className="text-xl font-bold text-purple-400">WindSurf Game Launcher</h1>
        <span className="text-xs text-gray-500">v{version}</span>
      </header>

      {/* Main Content Area (Scrollable if needed) */}
      <main className="flex-grow flex p-4 space-x-4 overflow-auto">

        {/* Left Panel: Game List (fixed width, scrollable) */}
        <aside className="w-64 flex-shrink-0 bg-gray-900 p-3 rounded-lg shadow-md overflow-y-auto">
          <h2 className="text-lg font-semibold mb-3 border-b border-gray-700 pb-2 text-purple-300">Library</h2>
          {games.length > 0 ? (
            games.map(game => (
              <GameItem
                key={game.id}
                game={game}
                isSelected={game.id === selectedGameId}
                onClick={() => setSelectedGameId(game.id)}
              />
            ))
          ) : (
            <p className="text-gray-500 text-sm text-center mt-4">No games found.</p>
          )}
        </aside>

        {/* Center Panel: Game Details (flex-grow) */}
        <section className="flex-grow bg-gray-900 rounded-lg shadow-md overflow-hidden">
          <GameDetails game={selectedGame} onAction={handleGameAction} />
        </section>

        {/* Right Panel: Auth & Actions (fixed width) */}
        <section className="w-72 flex-shrink-0 bg-gray-900 p-4 rounded-lg shadow-md flex flex-col space-y-4">
          <AuthArea
            authStatus={authStatus}
            onLogin={handleLogin} 
            onLogout={handleLogout}
            loginError={loginError}
          />
          {/* Placeholder for other actions/news */}
          <div className="bg-gray-700 p-3 rounded mt-auto shadow-inner">
            <h3 className="text-md font-semibold mb-2 text-purple-300">News Feed</h3>
            <p className="text-sm text-gray-400">Latest game updates and announcements will appear here.</p>
          </div>
        </section>
      </main>

      {/* Footer - Subtle */}
      <div className="px-6 py-1.5 bg-gray-950 border-t border-gray-700 text-xs text-gray-500 flex items-center justify-between flex-shrink-0">
        <span>Status: {api ? 'Ready' : 'Offline' }</span>
        <span>Version: {version}</span>
      </div>
    </div>
  );
}

export default App;