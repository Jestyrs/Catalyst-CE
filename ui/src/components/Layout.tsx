import React from 'react';
import TopBar from './TopBar';
import LeftSidebar from './LeftSidebar';
import RightSidebar from './RightSidebar';
import MainContentArea from './MainContentArea';

interface LayoutProps {
  // Add props for state and handlers needed by child components
  // Example:
  // games: Game[];
  // authStatus: AuthStatus | null;
  // onLogin: () => void;
}

const Layout: React.FC<LayoutProps> = (/* { other props } */) => {
  return (
    <div className="flex flex-col h-screen bg-gray-900 text-white">
      {/* Pass necessary props to TopBar */}
      <TopBar />

      <div className="flex flex-grow pt-12"> {/* pt-12 to offset fixed TopBar */}
        {/* Pass necessary props to LeftSidebar */}
        <LeftSidebar />

        {/* Pass necessary props to MainContentArea */}
        {/* If using children prop: <MainContentArea>{children}</MainContentArea> */}
        {/* Or pass specific components: */}
        <MainContentArea>
            {/* Content based on left sidebar selection will go here */} 
            {/* This might involve routing or conditional rendering managed by App.tsx */} 
        </MainContentArea>

        {/* Pass necessary props to RightSidebar */}
        <RightSidebar />
      </div>
    </div>
  );
};

export default Layout;
