import React from 'react';
import TopBar from './TopBar.tsx';
import LeftSidebar from './LeftSidebar.tsx';
import RightSidebar from './RightSidebar.tsx';
import MainContentArea from './MainContentArea.tsx';
import LoginModal from './LoginModal.tsx';

// --- Type Definitions (Copied from App.tsx/MainContentArea.tsx for now) ---
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

// --- Layout Component Props ---
interface LayoutProps {
  activeView: string;
  setActiveView: (view: string) => void;
  // Props for MainContentArea
  isLoadingGames: boolean;
  apiError: string | null;
  games: Game[];
  selectedGameId: string | null;
  setSelectedGameId: (id: string | null) => void;
  authStatus: AuthStatus | null;
  handleLogout: () => void;
  loginError: string | null;
  selectedGame: Game | null;
  handleGameAction: (gameId: string, action: string) => void;
  // Props needed specifically for TopBar
  handleLogin: () => void;
  // Props for Login Modal
  isLoginModalOpen: boolean;
  handleCloseLoginModal: () => void;
  handleLoginSubmit: (username: string, password: string) => Promise<void>;
}

const Layout: React.FC<LayoutProps> = ({
  activeView,
  setActiveView,
  // Destructure MainContentArea props
  isLoadingGames,
  apiError,
  games,
  selectedGameId,
  setSelectedGameId,
  authStatus,
  handleLogout,
  loginError,
  selectedGame,
  handleGameAction,
  // Destructure TopBar specific props
  handleLogin: handleLoginForTopBar,
  // Destructure Modal props
  isLoginModalOpen,
  handleCloseLoginModal,
  handleLoginSubmit
}) => {
  console.log('[Layout] Received authStatus:', authStatus);
  return (
    <div className="flex flex-col h-screen bg-zinc-900 text-gray-200">
      {/* Pass necessary props to TopBar */}
      <TopBar authStatus={authStatus} handleLogin={handleLoginForTopBar} />

      {/* Main content area: flex-grow allows this div to take remaining vertical space */}
      {/* pt-12 offsets the fixed TopBar (assuming h-12) */}
      {/* overflow-hidden prevents scrollbars on this row container itself */}
      <div className="flex flex-grow pt-12 overflow-hidden">
        {/* Left Sidebar: Fixed width, shrinks if needed but doesn't grow */}
        <div className="w-64 flex-shrink-0 bg-zinc-800">
          <LeftSidebar activeView={activeView} setActiveView={setActiveView} />
        </div>

        {/* Main Content: Takes remaining space */}
        <div className="flex-grow overflow-auto bg-zinc-900">
          <MainContentArea
            activeView={activeView} // Pass activeView too
            isLoadingGames={isLoadingGames}
            apiError={apiError}
            games={games}
            selectedGameId={selectedGameId}
            setSelectedGameId={setSelectedGameId}
            authStatus={authStatus}
            handleLogout={handleLogout}
            loginError={loginError}
            selectedGame={selectedGame}
            handleGameAction={handleGameAction}
          />
        </div>

        {/* Right Sidebar: Fixed width, shrinks if needed but doesn't grow */}
        <div className="w-80 flex-shrink-0 bg-zinc-800">
          <RightSidebar
            authStatus={authStatus}
            handleLogout={handleLogout}
          />
        </div>
      </div>
      {/* Render Login Modal inside Layout */}
      <LoginModal
        isOpen={isLoginModalOpen}
        onClose={handleCloseLoginModal}
        onSubmit={handleLoginSubmit}
        error={loginError} // Pass the loginError prop
        authStatus={authStatus?.status ?? 'LoggedOut'} // Pass authStatus status
      />
    </div>
  );
};

export default Layout;
