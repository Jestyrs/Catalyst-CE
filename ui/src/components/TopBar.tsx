import React from 'react';

// Placeholder components/icons - Remove placeholder bg colors
const AppLogo = () => <div className="w-6 h-6 rounded">L</div>; // Removed bg-blue-500
const ClanIcon = () => <div className="w-5 h-5 rounded-sm text-gray-400 hover:text-gray-100">C</div>; // Removed bg-green-500, added text styling
const UserAvatar = ({ loggedIn }: { loggedIn: boolean }) => (
  <div className={`w-8 h-8 rounded-full flex items-center justify-center ${loggedIn ? 'bg-purple-600' : 'bg-zinc-700'}`}> {/* Removed bg-purple-500, use purple only if logged in */} 
    {/* Placeholder or <img> */} U
  </div>
);
const NotificationIcon = () => <div className="w-5 h-5 rounded-sm text-gray-400 hover:text-gray-100">N</div>; // Removed bg-yellow-500, added text styling

interface TopBarProps {
  isAuthenticated: boolean;
  user: any | undefined; // Use a more specific type if available from Auth0
  onOpenLoginModal: () => void;
  onLogout: () => void;
}

const TopBar: React.FC<TopBarProps> = ({
  isAuthenticated,
  user,
  onOpenLoginModal,
  onLogout
}) => {
  console.log('[TopBar] Rendering. IsAuthenticated:', isAuthenticated);

  return (
    <div className="fixed top-0 left-0 right-0 z-10 flex justify-between items-center h-12 bg-zinc-800 text-gray-200 px-4 border-b border-zinc-700"> {/* Updated classes */}
      {/* Left Section: Window Controls + Logo */}
      <div className="flex items-center space-x-2">
        {/* Placeholder for Window Controls - Colors OK for dots */}
        <div className="flex space-x-1">
          <span className="w-3 h-3 bg-red-500 rounded-full"></span>
          <span className="w-3 h-3 bg-yellow-500 rounded-full"></span>
          <span className="w-3 h-3 bg-green-500 rounded-full"></span>
        </div>
        <AppLogo />
      </div>

      {/* Right Section: User Profile + Actions or Login Button */}
      <div className="flex items-center space-x-4">
        {isAuthenticated && user ? (
          <>
            <ClanIcon />
            <UserAvatar loggedIn={true} />
            <div className="text-sm">
              {/* Use user.name or user.nickname based on Auth0 profile */}
              <div>{user.name ?? user.nickname ?? 'User'}</div>
              <div className="text-xs text-gray-400">Online</div> {/* Updated text color */}
            </div>
            <NotificationIcon />
            {/* Settings Icon SVG Button - Add onClick handler if needed */}
            <button className="p-1 rounded hover:bg-zinc-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-zinc-800 focus:ring-zinc-500"> {/* Updated hover/focus */}
              <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth="1.5" stroke="currentColor" className="w-5 h-5"> {/* Ensure stroke='currentColor' */}
                <path strokeLinecap="round" strokeLinejoin="round" d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.325.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 0 1 1.37.49l1.296 2.247a1.125 1.125 0 0 1-.26 1.431l-1.003.827c-.293.24-.438.613-.43.992a6.76 6.76 0 0 1 0 1.918c-.008.378.137.75.43.99l1.005.828c.428.355.534.954.26 1.43l-1.298 2.247a1.125 1.125 0 0 1-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.47 6.47 0 0 1-.22.128c-.333.184-.582.496-.645.87l-.213 1.28c-.09.543-.56.94-1.11.94h-2.594c-.55 0-1.019-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 0 1-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 0 1-1.369-.49l-1.297-2.247a1.125 1.125 0 0 1 .26-1.431l1.004-.827c.292-.24.437-.613.43-.991a6.932 6.932 0 0 1 0-1.918c.008-.378-.137-.75-.43-.99l-1.004-.828a1.125 1.125 0 0 1-.26-1.43l1.297-2.247a1.125 1.125 0 0 1 1.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.086.22-.128.332-.184.582-.496.644-.87l.214-1.28Z" />
                <path strokeLinecap="round" strokeLinejoin="round" d="M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z" />
              </svg>
            </button>
            {/* Logout Button */}
            <button
              onClick={onLogout}
              className="px-3 py-1 bg-red-600 hover:bg-red-700 text-white text-xs font-semibold rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-zinc-800 focus:ring-red-500"
            >
              Logout
            </button>
          </>
        ) : (
          <>
            {/* Login with Username/Password Button */}
            <button
              onClick={onOpenLoginModal}
              className="px-3 py-1 bg-blue-600 hover:bg-blue-700 text-white text-xs font-semibold rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-zinc-800 focus:ring-blue-500"
            >
              Login
            </button>
            {/* Login with Google Button - REMOVED */}
            {/* 
            <button
              onClick={onLoginWithGoogle} 
              className="px-3 py-1 bg-green-600 hover:bg-green-700 text-white text-xs font-semibold rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-zinc-800 focus:ring-green-500"
            >
              Login with Google
            </button>
            */}
          </>
        )}
      </div>
    </div>
  );
};

export default TopBar;
