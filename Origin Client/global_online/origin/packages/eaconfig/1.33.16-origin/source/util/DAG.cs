using System;
using System.Collections.Generic;
using System.Text;
using NAnt.Core;

namespace EA.Eaconfig
{
public class DAGNode
{
    public readonly string Name;

    public bool Visited = false;
    public bool Completed = false;

    public readonly List<DAGNode> Dependencies = new List<DAGNode>();

    public DAGNode(string name)
    {
        Name = name;
    }

    public static List<DAGNode> Sort(List<DAGNode> dag)
    {
        List<DAGNode> result = new List<DAGNode>();
        foreach (DAGNode n in dag)
         {
             n.Visited = false;
             n.Completed = false;
         }

        foreach (DAGNode n in dag)
         {
             if (!n.Completed)
             {
                 Visit(n, result);
             }
         }
         return result;
    }

    private static bool Visit(DAGNode n, List<DAGNode> result)
    {
        if (n.Visited)
        {
            return n.Completed;
        }
        n.Visited = true;       
        foreach (DAGNode d in n.Dependencies)
        {
            if (!Visit(d, result))
            {                
                throw new BuildException("Circular dependency detected between '" + n.Name + "' and '" + d.Name + "'");
            }
        }
        n.Completed = true;
        result.Add(n);
        return true;
    }

}



}
