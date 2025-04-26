import React from 'react';

// Placeholder components/icons - Replace with actual implementations
const AppLogo = () => <div className="w-6 h-6 bg-blue-500 rounded">L</div>; // Simple placeholder
const ClanIcon = () => <div className="w-5 h-5 bg-green-500 rounded-sm">C</div>;
const UserAvatar = () => <div className="w-8 h-8 rounded-full bg-purple-500">U</div>;
const NotificationIcon = () => <div className="w-5 h-5 bg-yellow-500 rounded-sm">N</div>;
const SettingsIcon = () => <div className="w-5 h-5 bg-gray-500 rounded-sm">S</div>;

interface TopBarProps {
  // Add props for username, status, etc. from App.tsx
  // username?: string;
  // clanName?: string;
}

const TopBar: React.FC<TopBarProps> = (/* { username, clanName } */) => {
  return (
    <div className="fixed top-0 left-0 right-0 z-10 flex justify-between items-center h-12 bg-neutral-800 text-neutral-100 px-4 border-b border-neutral-700">
      {/* Left Section: Window Controls + Logo */}
      <div className="flex items-center space-x-2">
        {/* Placeholder for Window Controls */}
        <div className="flex space-x-1">
          <span className="w-3 h-3 bg-red-500 rounded-full"></span>
          <span className="w-3 h-3 bg-yellow-500 rounded-full"></span>
          <span className="w-3 h-3 bg-green-500 rounded-full"></span>
        </div>
        <AppLogo />
      </div>

      {/* Right Section: User Profile + Actions */}
      <div className="flex items-center space-x-4">
        <ClanIcon />
        <UserAvatar />
        <div className="text-sm">
          <div>Username</div> {/* Replace with prop */} 
          <div className="text-xs text-neutral-400">Online</div> {/* Add logic for status */}
        </div>
        <NotificationIcon />
        <SettingsIcon />
      </div>
    </div>
  );
};

export default TopBar;
