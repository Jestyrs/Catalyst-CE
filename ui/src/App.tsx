import React, { useState, useEffect } from 'react';

// --- Data Structures ---
interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'NotInstalled' | 'UpdateRequired' | 'Installing' | 'Downloading' | 'Launching' | 'Running' | 'Verifying';
  imageUrl?: string;
  description?: string;
  progress?: number;
  version?: string; // Added version
}

interface UserProfile {
    username: string;
    // Add other profile fields like avatarUrl, etc.
}

interface AuthStatus {
    status: 'Unknown' | 'LoggedOut' | 'LoggingIn' | 'LoggedIn' | 'LoggingOut' | 'Error';
    profile?: UserProfile;
    error?: string;
}

// --- Mock API ---
// Replace with actual backend communication logic
const mockApi = {
  getGameList: async (): Promise<Game[]> => {
    console.log("Mock API: getGameList called");
    await new Promise(resolve => setTimeout(resolve, 500));
    return [
      { id: "game1", name: "Cyber Odyssey", status: "NotInstalled", imageUrl: "https://placehold.co/600x400/1f2937/9ca3af?text=Cyber+Odyssey", description: "Explore a vast cyberpunk metropolis.", version: "1.2.0" },
      { id: "game2", name: "Stellar Conquest", status: "ReadyToPlay", imageUrl: "https://placehold.co/600x400/374151/9ca3af?text=Stellar+Conquest", description: "Conquer the galaxy in this epic space strategy game.", version: "3.1.5" },
      { id: "game3", name: "Mystic Realms", status: "UpdateRequired", imageUrl: "https://placehold.co/600x400/4b5563/9ca3af?text=Mystic+Realms", description: "Embark on a magical fantasy adventure.", version: "1.0.0" },
      { id: "game4", name: "Quantum Racer", status: "Downloading", progress: 35, imageUrl: "https://placehold.co/600x400/6b7280/111827?text=Quantum+Racer", description: "Race through dimensions at impossible speeds.", version: "2.0.1" },
      { id: "game5", name: "Project Chimera", status: "Installing", progress: 80, imageUrl: "https://placehold.co/600x400/9ca3af/111827?text=Project+Chimera", description: "Survive the genetic horrors.", version: "0.9.0" },
    ];
  },
  getAuthStatus: async (): Promise<AuthStatus> => {
    console.log("Mock API: getAuthStatus called");
    await new Promise(resolve => setTimeout(resolve, 300));
    // Simulate logged-out state initially
     return { status: 'LoggedOut' };
    // Simulate logged-in state:
    // return { status: 'LoggedIn', profile: { username: 'PlayerOne' } };
  },
   login: async (user: string, pass: string): Promise<AuthStatus> => {
     console.log("Mock API: login called for", user);
     await new Promise(resolve => setTimeout(resolve, 800));
     if (user === 'test' && pass === 'pw') {
       // Simulate successful login and return new status
       return { status: 'LoggedIn', profile: { username: 'PlayerTest' } };
     } else {
        // Simulate login failure
       return { status: 'Error', error: 'Invalid credentials' };
       // Or throw new Error("Invalid credentials");
     }
   },
   logout: async (): Promise<AuthStatus> => {
      console.log("Mock API: logout called");
      await new Promise(resolve => setTimeout(resolve, 400));
      // Simulate successful logout
      return { status: 'LoggedOut' };
   },
   performGameAction: async (gameId: string, action: string): Promise<{ status: string }> => {
      console.log(`Mock API: performGameAction called for ${gameId}, action: ${action}`);
      await new Promise(resolve => setTimeout(resolve, 600));
      // Simulate backend accepting the action
      return { status: `${action} initiated for ${gameId}` };
   },
   getVersion: async (): Promise<{ version: string }> => {
      console.log("Mock API: getVersion called");
      await new Promise(resolve => setTimeout(resolve, 100));
      return { version: "1.0.1-mock" };
   }
   // Add mock functions for settings if needed
};

// --- Components ---

// Inline SVG Icon Component (Example for Settings)
const SettingsIcon: React.FC<{ className?: string }> = ({ className }) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    strokeWidth="2"
    strokeLinecap="round"
    strokeLinejoin="round"
    className={className || "w-5 h-5"} // Default size
  >
    <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 .54 1.11l.12 1a2 2 0 0 1-.54 1.11l-.15.1a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.38a2 2 0 0 0-.73-2.73l-.15-.1a2 2 0 0 1-.54-1.11l-.12-1a2 2 0 0 1 .54-1.11l.15-.1a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z" />
    <circle cx="12" cy="12" r="3" />
  </svg>
);

// Simple Loading Spinner
const LoadingSpinner: React.FC = () => (
  <div className="flex justify-center items-center h-full p-10">
    <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-blue-500"></div>
  </div>
);

// Authentication Area Component
const AuthArea: React.FC<{
  authStatus: AuthStatus | null;
  onLogin: (user: string, pass: string) => Promise<void>;
  onLogout: () => Promise<void>;
}> = ({ authStatus, onLogin, onLogout }) => {
  const [user, setUser] = useState('');
  const [pass, setPass] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleLoginSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsLoading(true);
    setError(null); // Clear previous errors
    try {
      await onLogin(user, pass);
      // Parent component will update state based on login result
      // Only clear form if login attempt didn't immediately return an error state
      if (authStatus?.status !== 'Error') {
          setUser('');
          setPass('');
      }
    } catch (err: any) {
      // This catch block might not be needed if onLogin handles errors internally
      // and updates the authStatus prop with the error message.
      setError(err.message || "Login failed");
    } finally {
      setIsLoading(false);
    }
  };

  const handleLogoutClick = async () => {
     setIsLoading(true);
     setError(null);
     try {
        await onLogout();
     } catch (err: any) {
        setError(err.message || "Logout failed");
     } finally {
        setIsLoading(false);
     }
  }

  // Update local error state if the authStatus prop indicates an error
  useEffect(() => {
      setError(authStatus?.error || null);
  }, [authStatus]);

  if (authStatus?.status === 'LoggedIn' && authStatus.profile) {
    return (
      <div className="flex items-center space-x-3 text-sm">
        {/* Placeholder for Avatar */}
        {/* <img src={authStatus.profile.avatarUrl || 'default-avatar.png'} alt="User Avatar" className="w-6 h-6 rounded-full bg-gray-600"/> */}
        <span className="text-gray-300">Welcome, <span className="font-semibold text-blue-400">{authStatus.profile.username}</span>!</span>
        <button
          onClick={handleLogoutClick}
          disabled={isLoading}
          className="px-3 py-1 bg-red-600 hover:bg-red-700 rounded text-white text-xs font-medium transition duration-150 ease-in-out disabled:opacity-50 focus:outline-none focus:ring-2 focus:ring-red-500 focus:ring-offset-2 focus:ring-offset-gray-800"
        >
          {isLoading ? '...' : 'Logout'}
        </button>
      </div>
    );
  }

  return (
    <form onSubmit={handleLoginSubmit} className="flex items-center space-x-2 relative">
      <input
        type="text"
        value={user}
        onChange={(e) => setUser(e.target.value)}
        placeholder="Username"
        required
        className={`px-2 py-1 bg-gray-700 border ${error ? 'border-red-500' : 'border-gray-600'} rounded text-sm focus:outline-none focus:ring-1 focus:ring-blue-500 focus:border-blue-500 placeholder-gray-500`}
        aria-invalid={!!error}
        aria-describedby={error ? "login-error" : undefined}
      />
      <input
        type="password"
        value={pass}
        onChange={(e) => setPass(e.target.value)}
        placeholder="Password"
        required
        className={`px-2 py-1 bg-gray-700 border ${error ? 'border-red-500' : 'border-gray-600'} rounded text-sm focus:outline-none focus:ring-1 focus:ring-blue-500 focus:border-blue-500 placeholder-gray-500`}
        aria-invalid={!!error}
        aria-describedby={error ? "login-error" : undefined}
      />
      <button
        type="submit"
        disabled={isLoading || authStatus?.status === 'LoggingIn'}
        className="px-3 py-1 bg-blue-600 hover:bg-blue-700 rounded text-white text-sm font-medium transition duration-150 ease-in-out disabled:opacity-50 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 focus:ring-offset-gray-800"
      >
        {isLoading || authStatus?.status === 'LoggingIn' ? '...' : 'Login'}
      </button>
      {error && <p id="login-error" className="absolute top-full left-0 text-red-400 text-xs mt-1">{error}</p>}
    </form>
  );
};


// Game List Item Component
const GameListItem: React.FC<{ game: Game; isSelected: boolean; onSelect: (id: string) => void }> = ({ game, isSelected, onSelect }) => (
  <button
    onClick={() => onSelect(game.id)}
    className={`block w-full text-left px-4 py-3 rounded-md transition duration-150 ease-in-out focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 focus:ring-offset-gray-800 ${
      isSelected
        ? 'bg-gradient-to-r from-blue-600 to-blue-700 text-white shadow-md' // Enhanced selected state
        : 'text-gray-300 hover:bg-gray-700 hover:text-white'
    }`}
  >
    <span className="font-medium">{game.name}</span>
    {/* Optional: Add small status indicator */}
    {/* <span className={`ml-2 text-xs px-1.5 py-0.5 rounded ${game.status === 'ReadyToPlay' ? 'bg-green-600' : 'bg-gray-500'}`}>{game.status}</span> */}
  </button>
);

// Game Details Panel Component
const GameDetailsPanel: React.FC<{ game: Game | null }> = ({ game }) => {
  if (!game) {
    return (
      <div className="flex flex-col items-center justify-center h-full text-gray-500 p-6">
        <p className="text-lg">Select a game from the library</p>
      </div>
    );
  }

  return (
    <div className="p-8 space-y-5"> {/* Increased padding */}
      <h1 className="text-4xl font-bold text-white mb-2 tracking-tight">{game.name}</h1>
       <p className="text-sm text-gray-400 mb-4">Version: {game.version || 'N/A'}</p>
      {game.imageUrl && (
        <div className="aspect-video w-full rounded-lg shadow-xl overflow-hidden mb-6"> {/* Aspect ratio container */}
            <img
              src={game.imageUrl}
              alt={game.name}
              className="w-full h-full object-cover bg-gray-700" // Ensure image covers the container
              onError={(e) => { e.currentTarget.src = 'https://placehold.co/1600x900/4b5563/1f2937?text=Image+Load+Error'; }} // Higher res fallback
            />
        </div>
      )}
      <p className="text-gray-300 leading-relaxed">{game.description || "No description available."}</p>
      {/* Removed status from here, it's implied by the action button */}
      {/* Add more details like developer, publisher, release date etc. */}
    </div>
  );
};

// Action Button Component - Enhanced Styling
const ActionButton: React.FC<{ game: Game | null; onAction: (gameId: string, action: string) => void }> = ({ game, onAction }) => {
  if (!game) return (
      <button
          className="px-8 py-3 bg-gray-600 text-gray-400 rounded-md font-semibold text-lg shadow-md opacity-50 cursor-not-allowed"
          disabled
      >
          Select Game
      </button>
  );

  let actionText = 'Play';
  let actionType = 'launch';
  let baseBgColor = 'bg-green-600';
  let hoverBgColor = 'hover:bg-green-500';
  let ringColor = 'focus:ring-green-400';
  let disabled = false;

  switch (game.status) {
    case 'NotInstalled':
      actionText = 'Install';
      actionType = 'install';
      baseBgColor = 'bg-blue-600';
      hoverBgColor = 'hover:bg-blue-500';
      ringColor = 'focus:ring-blue-400';
      break;
    case 'ReadyToPlay':
      actionText = 'Play';
      actionType = 'launch';
      baseBgColor = 'bg-green-600';
      hoverBgColor = 'hover:bg-green-500';
      ringColor = 'focus:ring-green-400';
      break;
    case 'UpdateRequired':
      actionText = 'Update';
      actionType = 'update';
      baseBgColor = 'bg-yellow-600';
      hoverBgColor = 'hover:bg-yellow-500';
      ringColor = 'focus:ring-yellow-400';
      break;
    case 'Installing':
    case 'Downloading':
    case 'Verifying': // Added verifying state
      actionText = game.status;
      actionType = 'cancel';
      baseBgColor = 'bg-red-600';
      hoverBgColor = 'hover:bg-red-500';
      ringColor = 'focus:ring-red-400';
      break;
     case 'Launching':
       actionText = 'Launching...';
       actionType = 'none';
       baseBgColor = 'bg-gray-600';
       hoverBgColor = 'hover:bg-gray-600'; // No hover effect when disabled
       ringColor = 'focus:ring-gray-500';
       disabled = true;
       break;
     case 'Running':
       actionText = 'Running';
       actionType = 'none'; // Or maybe 'Focus' / 'Terminate'?
       baseBgColor = 'bg-gray-600';
       hoverBgColor = 'hover:bg-gray-600';
       ringColor = 'focus:ring-gray-500';
       disabled = true;
       break;
    default:
      actionText = 'Unknown';
      actionType = 'none';
      baseBgColor = 'bg-gray-600';
       hoverBgColor = 'hover:bg-gray-600';
       ringColor = 'focus:ring-gray-500';
      disabled = true;
  }

  return (
    <button
      onClick={() => actionType !== 'none' && onAction(game.id, actionType)}
      disabled={disabled}
      className={`px-8 py-3 ${baseBgColor} ${disabled ? '' : hoverBgColor} text-white font-semibold text-lg rounded-md shadow-lg transition duration-200 ease-in-out transform ${disabled ? '' : 'hover:-translate-y-0.5'} focus:outline-none focus:ring-2 ${ringColor} focus:ring-offset-2 focus:ring-offset-gray-800 disabled:opacity-70 disabled:cursor-not-allowed`}
    >
      {actionText}
    </button>
  );
};

// Progress Bar Component - Enhanced Styling
const ProgressBar: React.FC<{ game: Game | null }> = ({ game }) => {
  const showProgress = game && (game.status === 'Downloading' || game.status === 'Installing' || game.status === 'Verifying') && game.progress !== undefined;

  if (!showProgress) {
    return <div className="h-6"></div>; // Keep space consistent even when hidden
  }

  return (
    <div className="w-full space-y-1">
        <div className="w-full bg-gray-700 rounded-full h-2 overflow-hidden">
            <div
                className="bg-blue-500 h-2 rounded-full transition-all duration-300 ease-linear"
                style={{ width: `${game.progress}%` }}
            ></div>
        </div>
         <span className="text-xs text-gray-400 font-medium">{game.status} - {game.progress}%</span>
    </div>
  );
};


// Main App Component
const App: React.FC = () => {
  const [activeTab, setActiveTab] = useState<'library' | 'store'>('library');
  const [games, setGames] = useState<Game[]>([]);
  const [selectedGameId, setSelectedGameId] = useState<string | null>(null);
  const [isLoadingGames, setIsLoadingGames] = useState(true);
  const [authStatus, setAuthStatus] = useState<AuthStatus | null>(null);
  const [appVersion, setAppVersion] = useState<string>('Loading...');

  // Fetch initial data on mount
  useEffect(() => {
    const fetchData = async () => {
      setIsLoadingGames(true);
      setAuthStatus(null); // Indicate loading auth status
      try {
         const [gameList, authData, versionData] = await Promise.all([
            mockApi.getGameList(),
            mockApi.getAuthStatus(),
            mockApi.getVersion()
         ]);

        setGames(gameList);
        setAuthStatus(authData);
        setAppVersion(versionData.version);
        // Select the first game by default if list is not empty
        if (gameList.length > 0 && !selectedGameId) {
            setSelectedGameId(gameList[0].id);
        }

      } catch (error) {
        console.error("Failed to fetch initial data:", error);
        setAuthStatus({ status: 'Error', error: 'Failed to load initial data' });
      } finally {
        setIsLoadingGames(false);
      }
    };
    fetchData();

    // Setup Listener for Game Status Updates from Backend
    const handleGameStatusUpdate = (update: { game_id: string; status: string; progress?: number; message?: string }) => {
       console.log("Received game status update from backend:", update);
       setGames(currentGames =>
         currentGames.map(game =>
           game.id === update.game_id
             ? { ...game, status: update.status as Game['status'], progress: update.progress }
             : game
         )
       );
     };

     if (window.gameLauncherAPI) {
        window.gameLauncherAPI.onStatusUpdate = handleGameStatusUpdate;
        console.log("Game status update listener attached.");
     } else {
        console.warn("window.gameLauncherAPI not found. Status updates from backend will not be received.");
        // --- Simulate backend pushing updates for mock purposes ---
        const intervalId = setInterval(() => {
            setGames(currentGames => currentGames.map(g => {
                if (g.status === 'Downloading' || g.status === 'Installing') {
                    const newProgress = Math.min((g.progress || 0) + 10, 100);
                    return {
                        ...g,
                        progress: newProgress,
                        status: newProgress === 100 ? (g.status === 'Downloading' ? 'Verifying' : 'ReadyToPlay') : g.status
                    };
                }
                 if (g.status === 'Verifying') {
                     // Simulate verification taking a bit
                     const newProgress = Math.min((g.progress || 0) + 25, 100);
                      return {
                        ...g,
                        progress: newProgress,
                        status: newProgress === 100 ? 'ReadyToPlay' : g.status
                    };
                 }
                return g;
            }));
        }, 1500); // Update progress every 1.5 seconds
        return () => clearInterval(intervalId); // Cleanup interval
        // --- End mock simulation ---
     }

     // Cleanup listener on unmount
     return () => {
       if (window.gameLauncherAPI) {
         window.gameLauncherAPI.onStatusUpdate = undefined;
          console.log("Game status update listener removed.");
       }
     };

  }, []); // Empty dependency array means run once on mount

  const handleLogin = async (user: string, pass: string) => {
     setAuthStatus({ status: 'LoggingIn' }); // Show loading state
     const result = await mockApi.login(user, pass);
     setAuthStatus(result); // Update with result (success or error)
  };

  const handleLogout = async () => {
     setAuthStatus({ status: 'LoggingOut', profile: authStatus?.profile }); // Show loading state
     const result = await mockApi.logout();
     setAuthStatus(result); // Update with LoggedOut status
     setSelectedGameId(null); // Clear selection on logout? Optional.
  };

  const handleGameAction = async (gameId: string, action: string) => {
     console.log(`Performing action: ${action} for game: ${gameId}`);
     try {
       // Optimistic UI update
       const getNextStatus = (): Game['status'] => {
            switch(action) {
                case 'launch': return 'Launching';
                case 'install': return 'Downloading'; // Starts with download
                case 'update': return 'Downloading'; // Starts with download
                case 'cancel': return 'ReadyToPlay'; // Or 'NotInstalled' depending on previous state? Needs logic.
                default: return games.find(g => g.id === gameId)?.status || 'ReadyToPlay'; // Default to current status
            }
       }
       setGames(currentGames =>
            currentGames.map(g => g.id === gameId ? { ...g, status: getNextStatus(), progress: action === 'install' || action === 'update' ? 0 : undefined } : g)
       );

       await mockApi.performGameAction(gameId, action);
       // Let backend push actual progress/status via listener

     } catch (error: any) {
       console.error(`Failed to perform action ${action} for game ${gameId}:`, error);
       // TODO: Revert optimistic UI update & show error message
       // Example: Refetch game list or specific game status on error
       alert(`Action failed: ${error.message || 'Unknown error'}`); // Simple alert for now
       // Revert status (crude example, might need original status)
        setGames(currentGames =>
            currentGames.map(g => g.id === gameId ? { ...g, status: 'ReadyToPlay' } : g) // Revert to a known state
       );
     }
   };

  const selectedGame = games.find(game => game.id === selectedGameId) || null;

  return (
    // Main container - Slightly darker bg, ensure font is applied
    <div className="flex flex-col bg-gray-950 text-gray-300 min-h-screen font-sans antialiased">
      {/* Top Bar - Subtle gradient, slightly more padding */}
      <div className="flex items-center justify-between px-6 py-3 bg-gradient-to-r from-gray-800 to-gray-900 border-b border-gray-700 shadow-lg flex-shrink-0">
        {/* Tabs - More spacing, clearer active state */}
        <div className="flex space-x-2">
          <button
            onClick={() => setActiveTab('library')}
            className={`px-4 py-1.5 rounded-md text-sm font-semibold transition duration-200 ease-in-out focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 focus:ring-offset-gray-900 ${
              activeTab === 'library'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-gray-400 hover:bg-gray-700 hover:text-gray-100'
            }`}
          >
            Library
          </button>
          <button
            onClick={() => setActiveTab('store')}
            className={`px-4 py-1.5 rounded-md text-sm font-semibold transition duration-200 ease-in-out focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 focus:ring-offset-gray-900 ${
              activeTab === 'store'
                ? 'bg-blue-600 text-white shadow-sm'
                : 'text-gray-400 hover:bg-gray-700 hover:text-gray-100'
            }`}
          >
            Store
          </button>
        </div>

        {/* Auth & Settings Area */}
        <div className="flex items-center space-x-5">
           {authStatus === null ? (
              <span className="text-xs text-gray-500 animate-pulse">Loading user...</span>
           ) : (
              <AuthArea
                 authStatus={authStatus}
                 onLogin={handleLogin}
                 onLogout={handleLogout}
               />
           )}
          <button className="text-gray-400 hover:text-white transition duration-150 ease-in-out focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 focus:ring-offset-gray-900 rounded-full p-1" title="Settings">
            <SettingsIcon className="w-5 h-5" />
          </button>
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex flex-1 overflow-hidden">
        {/* Left Panel (Game List) - Slightly wider, more padding */}
        <div className="w-72 bg-gray-900 border-r border-gray-700 p-5 flex-shrink-0 overflow-y-auto space-y-2">
          <h2 className="text-xl font-bold mb-4 text-gray-100 tracking-tight">Library</h2>
          {isLoadingGames ? (
            <LoadingSpinner />
          ) : (
            games.map((game) => (
              <GameListItem
                key={game.id}
                game={game}
                isSelected={selectedGameId === game.id}
                onSelect={setSelectedGameId}
              />
            ))
          )}
        </div>

        {/* Right Panel (Details / Store Content) - Different background */}
        <div className="flex-1 bg-gray-850 overflow-y-auto"> {/* Slightly lighter than sidebar */}
          {activeTab === 'library' && (
            <div className="flex flex-col h-full">
               <div className="flex-1"> {/* Details take available space */}
                  <GameDetailsPanel game={selectedGame} />
               </div>
               {/* Action Bar - More padding, distinct background */}
               <div className="p-6 border-t border-gray-700 bg-gradient-to-r from-gray-800 to-gray-900 flex-shrink-0 shadow-inner">
                  <div className="flex items-center justify-between space-x-6">
                     {/* Action Button on the left */}
                     <ActionButton game={selectedGame} onAction={handleGameAction} />
                     {/* Progress Bar takes remaining space */}
                     <div className="flex-1 max-w-xs"> {/* Limit width of progress bar */}
                        <ProgressBar game={selectedGame} />
                     </div>
                  </div>
               </div>
            </div>
          )}
          {activeTab === 'store' && (
            <div className="p-8">
              <h1 className="text-3xl font-bold text-white">Store</h1>
              <p className="text-gray-400 mt-3">Browse and purchase new games.</p>
              {/* Add store items, categories, etc. here */}
            </div>
          )}
        </div>
      </div>

       {/* Footer - Subtle */}
       <div className="px-6 py-1.5 bg-gray-950 border-t border-gray-700 text-xs text-gray-500 flex items-center justify-between flex-shrink-0">
          <span>Status: Ready</span>
          <span>Version: {appVersion}</span>
       </div>
    </div>
  );
};

// --- Global types for window object (if needed for backend communication) ---
declare global {
  interface Window {
    gameLauncherAPI?: {
       getGameListRequest: () => Promise<Game[]>;
       getAuthStatusRequest: () => Promise<AuthStatus>;
       loginRequest: (user: string, pass: string) => Promise<AuthStatus>;
       logoutRequest: () => Promise<AuthStatus>;
       gameActionRequest: (gameId: string, action: string) => Promise<{ status: string }>;
       getVersionRequest: () => Promise<{ version: string }>;
       // Add functions for settings etc.
       onStatusUpdate?: (update: { game_id: string; status: string; progress?: number; message?: string }) => void;
    }
  }
}

export default App;