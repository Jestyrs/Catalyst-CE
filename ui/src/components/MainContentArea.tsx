import React from 'react';

interface MainContentAreaProps {
  children: React.ReactNode;
}

const MainContentArea: React.FC<MainContentAreaProps> = ({ children }) => {
  return (
    <main className="flex-grow p-6 overflow-y-auto bg-gray-700 ml-20 mr-64"> {/* ml-20 for LeftSidebar, mr-64 for RightSidebar */}
      {children}
    </main>
  );
};

export default MainContentArea;
