import React, { useState, useEffect } from 'react';

interface LoginModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSubmit: (username: string, password: string) => Promise<void>; // onSubmit now handles the async call
  error: string | null;
  authStatus: 'LoggingIn' | 'Error' | string; // Simplified for loading state indication
}

const LoginModal: React.FC<LoginModalProps> = ({ isOpen, onClose, onSubmit, error, authStatus }) => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [isSubmitting, setIsSubmitting] = useState(false);

  useEffect(() => {
    // Reset submission state if authStatus changes (e.g., from LoggingIn back to Error or something else)
    if (authStatus !== 'LoggingIn') {
      setIsSubmitting(false);
    }
  }, [authStatus]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (isSubmitting) return; // Prevent double submission

    setIsSubmitting(true);
    try {
      await onSubmit(username, password);
      // Success case: The modal will be closed by the parent component via onClose prop after successful submission
      // Reset local state only if needed (e.g., if modal stays open on error)
    } catch (err) {
      // Error is handled by the parent and passed via the 'error' prop.
      console.error("Error during login submission handled by parent:", err); 
    } finally {
      // We now rely on the authStatus prop change to reset isSubmitting
      // setIsSubmitting(false); // Removed this line
    }
  };

  if (!isOpen) {
    return null;
  }

  return (
    <div 
      className="fixed inset-0 z-50 flex items-center justify-center bg-black bg-opacity-60 backdrop-blur-sm" 
      onClick={onClose} // Close if clicking outside the modal content
    >
      <div 
        className="bg-zinc-800 text-gray-200 p-8 rounded-lg shadow-xl w-full max-w-md relative"
        onClick={(e) => e.stopPropagation()} // Prevent click inside modal from closing it
      >
        {/* Close Button */}
        <button 
          onClick={onClose} 
          className="absolute top-2 right-2 text-gray-400 hover:text-gray-100 text-2xl font-bold p-1 leading-none"
          aria-label="Close"
        >
          &times;
        </button>

        <h2 className="text-2xl font-bold mb-6 text-center text-indigo-400">Login</h2>
        
        <form onSubmit={handleSubmit}>
          {/* Error Display */}
          {error && (
            <div className="bg-red-800 border border-red-600 text-red-100 px-4 py-3 rounded relative mb-4" role="alert">
              <strong className="font-bold">Error:</strong>
              <span className="block sm:inline ml-2">{error}</span>
            </div>
          )}

          {/* Username Input */}
          <div className="mb-4">
            <label htmlFor="username" className="block text-sm font-medium text-gray-300 mb-1">Username</label>
            <input 
              type="text" 
              id="username"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              required 
              className="w-full px-3 py-2 bg-zinc-700 border border-zinc-600 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 placeholder-gray-500 text-white"
              placeholder="Enter your username"
            />
          </div>

          {/* Password Input */}
          <div className="mb-6">
            <label htmlFor="password" className="block text-sm font-medium text-gray-300 mb-1">Password</label>
            <input 
              type="password" 
              id="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              required
              className="w-full px-3 py-2 bg-zinc-700 border border-zinc-600 rounded-md focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500 placeholder-gray-500 text-white"
              placeholder="Enter your password"
            />
          </div>

          {/* Submit Button */}
          <button 
            type="submit" 
            disabled={isSubmitting || authStatus === 'LoggingIn'} // Disable while submitting
            className={`w-full py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 focus:ring-offset-zinc-800 disabled:opacity-50 disabled:cursor-not-allowed transition duration-150 ease-in-out`}
          >
            {isSubmitting || authStatus === 'LoggingIn' ? 'Logging In...' : 'Login'}
          </button>
        </form>
      </div>
    </div>
  );
};

export default LoginModal;
