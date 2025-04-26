import React from 'react';

// --- Type Definitions (Copied from App.tsx for now) ---
interface Game {
  id: string;
  name: string;
  status: 'ReadyToPlay' | 'Downloading' | 'Installing' | 'UpdateRequired' | 'NotInstalled' | 'Verifying' | 'Repairing';
  imageUrl: string;
  description: string;
  version: string;
  progress?: number;
}

interface UserProfile {
  username: string;
  // Add other profile details
}

type AuthStatus =
  | { status: 'LoggedIn'; profile: UserProfile }
  | { status: 'LoggedOut' }
  | { status: 'LoggingIn' }
  | { status: 'LoggingOut' }
  | { status: 'Error'; error?: string; profile?: UserProfile | null };

// --- Child Components (Copied from App.tsx for now) ---

// Game Item Component
const GameItem: React.FC<{ game: Game; isSelected: boolean; onClick: () => void }> = React.memo(({ game, isSelected, onClick }) => {
  return (
    <div
      className={`flex items-center p-3 rounded-lg mb-2 cursor-pointer transition-colors duration-150 ease-in-out ${isSelected ? 'bg-neutral-600 shadow-md' : 'bg-neutral-700 hover:bg-neutral-600'}`}
      onClick={onClick}
    >
      <img src={game.imageUrl} alt={game.name} className="w-16 h-10 object-cover rounded mr-4" />
      <div className="flex-grow">
        <h3 className={`font-semibold ${isSelected ? 'text-neutral-100' : 'text-neutral-200'}`}>{game.name}</h3>
        <p className={`text-xs ${isSelected ? 'text-neutral-300' : 'text-neutral-400'}`}>{game.status}</p>
      </div>
    </div>
  );
});

// Game Details Component
const GameDetails: React.FC<{ game: Game | null; onAction: (gameId: string, action: string) => void }> = ({ game, onAction }) => {
  if (!game) {
    return <div className="text-center text-neutral-500 p-10">Select a game to see details</div>;
  }

  const renderActionButton = () => {
    switch (game.status) {
      case 'ReadyToPlay':
        return <button onClick={() => onAction(game.id, 'launch')} className="w-full bg-neutral-600 hover:bg-neutral-500 text-neutral-100 font-bold py-2 px-4 rounded transition duration-150 ease-in-out">Launch</button>;
      case 'UpdateRequired':
        return <button onClick={() => onAction(game.id, 'update')} className="w-full bg-neutral-600 hover:bg-neutral-500 text-neutral-100 font-bold py-2 px-4 rounded transition duration-150 ease-in-out">Update</button>;
      case 'NotInstalled':
        return <button onClick={() => onAction(game.id, 'install')} className="w-full bg-neutral-600 hover:bg-neutral-500 text-neutral-100 font-bold py-2 px-4 rounded transition duration-150 ease-in-out">Install</button>;
      case 'Downloading':
      case 'Installing':
      case 'Verifying':
      case 'Repairing':
        return (
          <div className="w-full bg-neutral-600 rounded h-8 flex items-center justify-center relative overflow-hidden">
            <div className="absolute left-0 top-0 h-full bg-neutral-500 transition-all duration-300 ease-linear" style={{ width: `${game.progress || 0}%` }}></div>
            <span className="relative z-10 text-neutral-100 text-sm font-medium">{game.status}... {game.progress || 0}%</span>
          </div>
        );
      default:
        return <button className="w-full bg-neutral-500 text-neutral-300 font-bold py-2 px-4 rounded cursor-not-allowed" disabled>Unavailable</button>;
    }
  };

  return (
    <div className="flex flex-col h-full bg-neutral-800 rounded-lg overflow-hidden shadow-inner">
      <img src={game.imageUrl} alt={game.name} className="w-full h-48 object-cover" />
      <div className="p-6 flex-grow flex flex-col">
        <h2 className="text-2xl font-bold mb-2 text-neutral-100">{game.name}</h2>
        <p className="text-neutral-400 text-sm mb-4 flex-grow">{game.description}</p>
        <div className="flex justify-between items-center text-xs text-neutral-500 mb-4">
          <span>Version: {game.version}</span>
          {/* Add other details like playtime, last played etc. */}
        </div>
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
  return (
    <div className="text-sm">
      {loginError && (
        <p className="text-red-400 bg-red-900 border border-red-700 p-2 rounded mb-3 text-center">Error: {loginError}</p>
      )}
      {authStatus?.status === 'LoggedIn' && (
        <div className="flex flex-col items-center space-y-2">
          <p className="text-neutral-300">Welcome, <span className="font-semibold text-neutral-100">{authStatus.profile.username}</span>!</p>
          <button
            onClick={onLogout}
            className="w-full bg-red-600 hover:bg-red-700 text-white font-bold py-2 px-4 rounded transition duration-150 ease-in-out"
          >
            Logout
          </button>
        </div>
      )}
      {authStatus?.status === 'LoggedOut' && (
        <button
          onClick={onLogin}
          className="w-full bg-neutral-600 hover:bg-neutral-500 text-neutral-100 font-bold py-2 px-4 rounded transition duration-150 ease-in-out"
        >
          Login
        </button>
      )}
      {authStatus?.status === 'LoggingIn' && <p className="text-neutral-400 text-center">Logging in...</p>}
      {authStatus?.status === 'LoggingOut' && <p className="text-neutral-400 text-center">Logging out...</p>}
      {authStatus?.status === 'Error' && authStatus.error && !loginError && (
          <p className="text-red-400 bg-red-900 border border-red-700 p-2 rounded mb-3 text-center">Session Error: {authStatus.error}</p>
      )}
      {/* Optionally handle Error state by showing login button if profile is missing? */}
    </div>
  );
};

interface MainContentAreaProps {
  activeView: string;
  isLoadingGames: boolean;
  apiError: string | null;
  games: Game[];
  selectedGameId: string | null;
  setSelectedGameId: (id: string | null) => void;
  authStatus: AuthStatus | null;
  handleLogin: () => void;
  handleLogout: () => void;
  loginError: string | null;
  selectedGame: Game | null;
  handleGameAction: (gameId: string, action: string) => void;
}

const MainContentArea: React.FC<MainContentAreaProps> = ({
  activeView,
  isLoadingGames,
  apiError,
  games,
  selectedGameId,
  setSelectedGameId,
  authStatus,
  handleLogin,
  handleLogout,
  loginError,
  selectedGame,
  handleGameAction
}) => {
  return (
    <main className="flex-grow p-6 overflow-y-auto bg-neutral-700"> 
       {isLoadingGames ? (
          <div className="flex justify-center items-center h-full">
            <p className="text-xl text-neutral-400">Loading Launcher Data...</p>
          </div>
        ) : apiError ? (
          <div className="flex justify-center items-center h-full p-10">
            <p className="text-xl text-red-400 bg-red-900 p-4 rounded border border-red-600">Error: {apiError}</p>
          </div>
        ) : activeView === 'Games' ? (
          <div className="grid grid-cols-3 gap-6 h-full">
            <div className="col-span-1 flex flex-col h-full overflow-hidden">
              <h2 className="text-xl font-semibold mb-4 px-3 pt-3 text-neutral-200">My Games ({games.length})</h2>
              <div className="flex-grow overflow-y-auto pr-2 space-y-2 no-scrollbar pl-3 pb-3">
                {games.length > 0 ? (
                  games.map(game => (
                    <GameItem
                      key={game.id}
                      game={game}
                      isSelected={selectedGameId === game.id}
                      onClick={() => setSelectedGameId(game.id)}
                    />
                  ))
                ) : (
                  <p className="text-neutral-500 text-center py-10">No games found.</p>
                )}
              </div>
              <div className="mt-auto p-3 border-t border-neutral-700">
                <AuthArea
                  authStatus={authStatus}
                  onLogin={handleLogin}
                  onLogout={handleLogout}
                  loginError={loginError}
                />
              </div>
            </div>

            <div className="col-span-2 h-full overflow-hidden">
              <GameDetails game={selectedGame} onAction={handleGameAction} />
            </div>
          </div>
        ) : activeView === 'Home' ? (
          <div className="text-center p-10 text-neutral-400">Home View Placeholder</div>
        ) : activeView === 'Shop' ? (
          <div className="text-center p-10 text-neutral-400">Shop View Placeholder</div>
        ) : activeView === 'Clans' ? (
          <div className="text-center p-10 text-neutral-400">Clans View Placeholder</div>
        ) : (
          <div className="text-center p-10 text-neutral-400">Unknown View: {activeView}</div> // Show activeView if unknown
        )}
    </main>
  );
};

export default MainContentArea;
