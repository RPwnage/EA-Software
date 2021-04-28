using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
public class DAGNode<T>
{
    public readonly T Value;

    public bool Visited = false;
    public bool Completed = false;

    public readonly List<DAGNode<T>> Dependencies = new List<DAGNode<T>>();

    public DAGNode(T value)
    {
        Value = value;
    }

    public static List<DAGNode<T>> Sort(IEnumerable<DAGNode<T>> dag, Action<DAGNode<T>, DAGNode<T>> onCircular = null )
    {
        var result = new List<DAGNode<T>>();
        foreach (var n in dag)
         {
             n.Visited = false;
             n.Completed = false;
         }

        foreach (var n in dag)
         {
             if (!n.Completed)
             {
                 Visit(n, result, onCircular);
             }
         }
         return result;
    }

    private static bool Visit(DAGNode<T> n, List<DAGNode<T>> result, Action<DAGNode<T>, DAGNode<T>> onCircular)
    {
        if (n.Visited)
        {
            return n.Completed;
        }
        n.Visited = true;
        foreach (var d in n.Dependencies)
        {
            if (!Visit(d, result, onCircular))
            {
                if (onCircular != null)
                {
                    onCircular(n, d);
                }
                else
                {
                    throw new BuildException("Circular dependency detected between '" + n.ToString() + "' and '" + d.ToString() + "'");
                }
            }
        }
        n.Completed = true;
        result.Add(n);
        return true;
    }
}



}
