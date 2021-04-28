using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Util
{
    public struct OnExit : IDisposable
    {
        public OnExit(Action onExit)
        {
            this.onExit = onExit;
        }

        public void Dispose()
        {
            if (onExit != null)
                onExit();
            onExit = null;
        }

        private Action onExit;
    }
}
