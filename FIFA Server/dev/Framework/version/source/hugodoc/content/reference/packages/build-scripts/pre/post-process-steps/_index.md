---
title: Pre/Post Process Steps
weight: 96
---

Sections in this topic describe what pre and postprocess steps are and how to use them

<a name="PrePostProcessSteps"></a>
## Pre and Post Process Steps in Framework ##

Pre and Post Process Steps refer to a number of different types of steps that can occur before or after
certain points of build script processing, prior to actual nant build execution.
These can come in the form of Build graph processing that happens before/after processing the entire buildgraph,
package preprocessing that occurs before/after processing the package,
module preprocessing that occurs before/after processing the build script of a module definition,
or pre-generate steps that occur before/after generating a visual studio solution.

<a name="ProcessStepsExecutionOrder"></a>
## Execution Order ##

Order of execution of processing steps in Framework

|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ![Process Steps Order]( processstepsorder.png ) | |   |   |<br>|---|---|<br>|  [(1)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/packagelevelprocess.md" >}} )  |  `package.preprocess`  |<br>|  [(2)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/globalpreprocess.md" >}} )  |    |<br>|  [(3)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/moduleprocess.md" >}} )  |  `eaconfig.preprocess`  |<br>|  [(4)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/moduleprocess.md" >}} )  |  `eaconfig.postprocess`  |<br>|  **(5)**  |    |<br>|  [(6)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/globalpostprocess.md" >}} )  |    |<br>|  [(7)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/pregeneratestep.md" >}} )  |  `backend.VisualStudio.pregenerate`  |<br>|  [(8)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/postgeneratestep.md" >}} )  |  `backend.VisualStudio.postgenerate`  |<br><br> |  [(1)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/packagelevelprocess.md" >}} )  |  `package.preprocess`  |  [(2)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/globalpreprocess.md" >}} )  |    |  [(3)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/moduleprocess.md" >}} )  |  `eaconfig.preprocess`  |  [(4)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/moduleprocess.md" >}} )  |  `eaconfig.postprocess`  |  **(5)**  |    |  [(6)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/globalpostprocess.md" >}} )  |    |  [(7)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/pregeneratestep.md" >}} )  |  `backend.VisualStudio.pregenerate`  |  [(8)]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/postgeneratestep.md" >}} )  |  `backend.VisualStudio.postgenerate`  |


##### Related Topics: #####
-  [See BuildSteps example]( {{< ref "reference/examples.md" >}} ) 
