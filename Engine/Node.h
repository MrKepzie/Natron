/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_ENGINE_NODE_H
#define NATRON_ENGINE_NODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#include <map>
#include <list>
#include <bitset>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QMetaType>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QMutex>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include "Engine/AppManager.h"
#include "Global/KeySymbols.h"
#include "Engine/ImageComponents.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define NATRON_PARAMETER_PAGE_NAME_EXTRA "Node"
#define NATRON_PARAMETER_PAGE_NAME_INFO "Info"


#define kDisableNodeKnobName "disableNode"
#define kUserLabelKnobName "userTextArea"
#define kEnableMaskKnobName "enableMask"
#define kEnableInputKnobName "enableInput"
#define kMaskChannelKnobName "maskChannel"
#define kInputChannelKnobName "inputChannel"
#define kEnablePreviewKnobName "enablePreview"
#define kOutputChannelsKnobName "channels"

#define kOfxMaskInvertParamName "maskInvert"
#define kOfxMixParamName "mix"

#define kReadOIIOAvailableViewsKnobName "availableViews"
#define kWriteOIIOParamViewsSelector "viewsSelector"

#define kWriteParamFrameStep "frameIncr"
#define kWriteParamFrameStepLabel "Frame Increment"
#define kWriteParamFrameStepHint "The number of frames the timeline should step before rendering the new frame. " \
"If 1, all frames will be rendered, if 2 only 1 frame out of 2, " \
"etc. This number cannot be less than 1."



NATRON_NAMESPACE_ENTER;

class Node
    : public QObject, public boost::enable_shared_from_this<Node>
    , public CacheEntryHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct Implementation;


    Node(AppInstance* app,
         const boost::shared_ptr<NodeCollection>& group,
         Plugin* plugin);

    virtual ~Node();

    boost::shared_ptr<NodeCollection> getGroup() const;
    
    const Plugin* getPlugin() const;
    
    /**
     * @brief Used internally when instanciating a Python template, we first make a group and then pass a pointer
     * to the real plugin.
     **/
    void switchInternalPlugin(Plugin* plugin);
    
    void setPrecompNode(const boost::shared_ptr<PrecompNode>& precomp);
    boost::shared_ptr<PrecompNode> isPartOfPrecomp() const;
    
    /**
     * @brief Creates the EffectInstance that will be embedded into this node and set it up.
     * This function also loads all parameters. Node connections will not be setup in this method.
     **/
    void load(const CreateNodeArgs& args);
    


    ///called by load() and OfxEffectInstance, do not call this!
    void loadKnobs(const NodeSerialization & serialization,bool updateKnobGui = false);


private:
    void loadKnob(const KnobPtr & knob,const std::list< boost::shared_ptr<KnobSerialization> > & serialization,
                  bool updateKnobGui = false);
public:
    
    ///Set values for Knobs given their serialization
    void setValuesFromSerialization(const std::list<boost::shared_ptr<KnobSerialization> >& paramValues);
   
    ///to be called once all nodes have been loaded from the project or right away after the load() function.
    ///this is so the child of a multi-instance can retrieve the pointer to it's main instance
    void fetchParentMultiInstancePointer();


    ///If the node can have a roto context, create it
    void createRotoContextConditionnally();

    ///function called by EffectInstance to create a knob
    template <class K>
    boost::shared_ptr<K> createKnob(const std::string &description,
                                    int dimension = 1,
                                    bool declaredByPlugin = true)
    {
        return appPTR->getKnobFactory().createKnob<K>(getEffectInstance(),description,dimension,declaredByPlugin);
    }

    ///This cannot be done in loadKnobs as to call this all the nodes in the project must have
    ///been loaded first.
    void restoreKnobsLinks(const NodeSerialization & serialization,
                           const NodesList & allNodes,
                           const std::map<std::string,std::string>& oldNewScriptNamesMapping);
    
    void restoreUserKnobs(const NodeSerialization& serialization);
    
    void setPagesOrder(const std::list<std::string>& pages);
    
    std::list<std::string> getPagesOrder() const;
    
    bool isNodeCreated() const;
    
    void refreshAcceptedBitDepths();
    
    /*@brief Quit all processing done by all render instances of this node
       This is called when the effect is about to be deleted pluginsly
     */
    void setMustQuitProcessing(bool mustQuit);
    
    /**
     * @brief Quits any processing on going on this node and waits until done
     * After this call all threads launched by this node are stopped.
     * This is called when clearing all nodes of the project (see Project::reset) or when calling
     * AppManager::abortAnyProcessing()
     **/
    void quitAnyProcessing();

    /* @brief Similar to quitAnyProcessing except that the threads aren't destroyed
     * This is called when a node is deleted by the user
     */
    void abortAnyProcessing();

    /*Never call this yourself. This is needed by OfxEffectInstance so the pointer to the live instance
     * is set earlier.
     */
    void setEffect(const EffectInstPtr& liveInstance);

    EffectInstPtr getEffectInstance() const;

    /**
     * @brief Returns true if the node is a multi-instance node, that is, holding several other nodes.
     * e.g: the Tracker node.
     **/
    bool isMultiInstance() const;
    
    NodePtr getParentMultiInstance() const;

    ///Accessed by the serialization thread, but mt safe since never changed
    std::string getParentMultiInstanceName() const;
    
    void getChildrenMultiInstance(NodesList* children) const;

    /**
     * @brief Returns the hash value of the node, or 0 if it has never been computed.
     **/
    U64 getHashValue() const;

    virtual std::string getCacheID() const OVERRIDE FINAL;

    /**
     * @brief Forwarded to the live effect instance
     **/
    void beginEditKnobs();

    /**
     * @brief Forwarded to the live effect instance
     **/
    const std::vector< KnobPtr > & getKnobs() const;

    /**
     * @brief When frozen is true all the knobs of this effect read-only so the user can't interact with it.
     * @brief This function will be called on all input nodes aswell
     **/
    void setKnobsFrozen(bool frozen);


    /*Returns in viewers the list of all the viewers connected to this node*/
    void hasViewersConnected(std::list<ViewerInstance* >* viewers) const;

    void hasOutputNodesConnected(std::list<OutputEffectInstance* >* writers) const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMajorVersion() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMinorVersion() const;


    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isInputNode() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isOutputNode() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isOpenFXNode() const;

    /**
     * @brief Returns true if the node is either a roto  node
     **/
    bool isRotoNode() const;
    
    /**
     * @brief Returns true if this node is a tracker
     **/
    bool isTrackerNode() const;
    
    bool isPointTrackerNode() const;

    /**
     * @brief Returns true if this node is a backdrop
     **/
    bool isBackdropNode() const;
    
    /**
     * @brief Returns true if the node is a rotopaint node
     **/
    bool isRotoPaintingNode() const;
    
    ViewerInstance* isEffectViewer() const;
    NodeGroup* isEffectGroup() const;

    /**
     * @brief Returns a pointer to the rotoscoping context if the node is in the paint context, otherwise NULL.
     **/
    boost::shared_ptr<RotoContext> getRotoContext() const;

    U64 getRotoAge() const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMaxInputCount() const;

    /**
     * @brief Returns true if the given input supports the given components. If inputNb equals -1
     * then this function will check whether the effect can produce the given components.
     **/
    bool isSupportedComponent(int inputNb,const ImageComponents& comp) const;

    /**
     * @brief Returns the most appropriate components that can be supported by the inputNb.
     * If inputNb equals -1 then this function will check the output components.
     **/
    ImageComponents findClosestSupportedComponents(int inputNb,const ImageComponents& comp) const;
    static ImageComponents findClosestInList(const ImageComponents& comp,
                                                     const std::list<ImageComponents> &components,
                                                     bool multiPlanar);
    
    ImageBitDepthEnum getBestSupportedBitDepth() const;
    bool isSupportedBitDepth(ImageBitDepthEnum depth) const;
    ImageBitDepthEnum getClosestSupportedBitDepth(ImageBitDepthEnum depth);

    /**
     * @brief Returns the components and index of the channel to use to produce the mask.
     * None = -1
     * R = 0
     * G = 1
     * B = 2
     * A = 3
     **/
    int getMaskChannel(int inputNb,ImageComponents* comps, NodePtr* maskInput) const;

    /**
     * @brief Returns whether masking is enabled or not
     **/
    bool isMaskEnabled(int inputNb) const;

    /**
     * @brief Returns a pointer to the input Node at index 'index'
     * or NULL if it couldn't find such node.
     * MT-safe
     * This function uses thread-local storage because several render thread can be rendering concurrently
     * and we want the rendering of a frame to have a "snap-shot" of the tree throughout the rendering of the
     * frame.
     *
     * DO NOT CALL THIS ON THE SERIALIZATION THREAD, INSTEAD PREFER USING getInputNames()
     **/
    NodePtr getInput(int index) const;
    
    /**
     * @brief Returns the input as seen on the gui. This is not necessarily the same as the value returned by getInput.
     **/
    NodePtr getGuiInput(int index) const;
    
    /**
     * @brief Same as getInput except that it doesn't do group redirections for Inputs/Outputs
     **/
    NodePtr getRealInput(int index) const;
    
    NodePtr getRealGuiInput(int index) const;
    
private:
    
    NodePtr getInputInternal(bool useGuiInput, bool useGroupRedirections, int index) const;
    
public:
    
    /**
     * @brief Returns the input index of the node if it is an input of this node, -1 otherwise.
     **/
    int getInputIndex(const Node* node) const;

    /**
     * @brief Returns true if the node is currently executing the onInputChanged handler.
     **/
    bool duringInputChangedAction() const;

    /**
     *@brief Returns the inputs of the node as the Gui just set them.
     * The vector might be different from what getInputs_other_thread() could return.
     * This can only be called by the main thread.
     **/
    const std::vector<NodeWPtr > & getInputs() const WARN_UNUSED_RETURN;
    const std::vector<NodeWPtr > & getGuiInputs() const WARN_UNUSED_RETURN;
    std::vector<NodeWPtr > getInputs_copy() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the input index of the node n if it exists,
     * -1 otherwise.
     **/
    int inputIndex(const NodePtr& n) const;

    const std::vector<std::string> & getInputLabels() const;
    std::string getInputLabel(int inputNb) const;
    
    int getInputNumberFromLabel(const std::string& inputLabel) const;

    bool isInputConnected(int inputNb) const;

    bool hasOutputConnected() const;

    bool hasInputConnected() const;
    
    bool hasOverlay() const;
    
    bool hasMandatoryInputDisconnected() const;
    
    bool hasAllInputsConnected() const;
    
    /**
     * @brief This is used by the auto-connection algorithm.
     * When connecting nodes together this function helps determine
     * on which input it should connect a new node.
     * It returns the first non optional empty input or the first optional
     * empty input if they are all optionals, or -1 if nothing matches the 2 first conditions..
     * if all inputs are connected.
     **/
    virtual int getPreferredInputForConnection()  const;
    virtual int getPreferredInput() const;
    
    NodePtr getPreferredInputNode() const;
    
    void setRenderThreadSafety(RenderSafetyEnum safety);
    RenderSafetyEnum getCurrentRenderThreadSafety() const;
    void revertToPluginThreadSafety();
    
    void setCurrentOpenGLRenderSupport(PluginOpenGLRenderSupport support);
    PluginOpenGLRenderSupport getCurrentOpenGLRenderSupport() const;
    
    void setCurrentSequentialRenderSupport(SequentialPreferenceEnum support);
    SequentialPreferenceEnum getCurrentSequentialRenderSupport() const;
    
    void setCurrentCanTransform(bool support);
    bool getCurrentCanTransform() const;
    
    void setCurrentSupportTiles(bool support);
    bool getCurrentSupportTiles() const;
    
    void refreshDynamicProperties();
    
    /////////////////////ROTO-PAINT related functionnalities//////////////////////
    //////////////////////////////////////////////////////////////////////////////
    
    void prepareForNextPaintStrokeRender();
    
    //Used by nodes below the rotopaint tree to optimize the RoI
    void setLastPaintStrokeDataNoRotopaint();
    void invalidateLastPaintStrokeDataNoRotopaint();
    
    void getPaintStrokeRoD(double time,RectD* bbox) const;
    RectD getPaintStrokeRoD_duringPainting() const;
    
    bool isLastPaintStrokeBitmapCleared() const;
    void clearLastPaintStrokeRoD();
    void getLastPaintStrokePoints(double time,std::list<std::list<std::pair<Point,double> > >* strokes, int* strokeIndex) const;
    boost::shared_ptr<Image> getOrRenderLastStrokeImage(unsigned int mipMapLevel,
                                                                double par,
                                                                const ImageComponents& components,
                                                                ImageBitDepthEnum depth) const;
    
    void setWhileCreatingPaintStroke(bool creating);
    bool isDuringPaintStrokeCreation() const;
    ////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////
    
    void setProcessChannelsValues(bool doR,bool doG, bool doB, bool doA);
    
private:
    
    int getPreferredInputInternal(bool connected) const;
    
public:
    
    
    /**
     * @brief Returns in 'outputs' a map of all nodes connected to this node
     * where the value of the map is the input index from which these outputs
     * are connected to this node.
     **/
    void getOutputsConnectedToThisNode(std::map<NodePtr,int>* outputs);

    const NodesWList & getOutputs() const;
    const NodesWList & getGuiOutputs() const;
    void getOutputs_mt_safe(NodesWList& outputs) const;
    
    /**
     * @brief Same as above but enters into subgroups
     **/
    void getOutputsWithGroupRedirection(NodesList& outputs) const;

    /**
     * @brief Each input name is appended to the vector, in the same order
     * as they are in the internal inputs vector. Disconnected inputs are
     * represented as empty strings.
     **/
    void getInputNames(std::map<std::string,std::string> & inputNames) const;
    
    enum CanConnectInputReturnValue
    {
        eCanConnectInput_ok = 0,
        eCanConnectInput_indexOutOfRange,
        eCanConnectInput_inputAlreadyConnected,
        eCanConnectInput_givenNodeNotConnectable,
        eCanConnectInput_groupHasNoOutput,
        eCanConnectInput_graphCycles,
        eCanConnectInput_differentPars,
        eCanConnectInput_differentFPS,
        eCanConnectInput_multiResNotSupported,
    };
    /**
     * @brief Returns true if a connection is possible for the given input number of the current node 
     * to the given input.
     **/
    Node::CanConnectInputReturnValue canConnectInput(const NodePtr& input,int inputNumber) const;
    

    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    virtual bool connectInput(const NodePtr& input, int inputNumber);

    bool connectInputBase(const NodePtr& input, int inputNumber)
    {
        return Node::connectInput(input, inputNumber);
    }
    
    /** @brief Removes the node connected to the input inputNumber of the
     * node. Returns the inputNumber if it could remove it, otherwise returns
       -1.
     */
    virtual int disconnectInput(int inputNumber);

    /** @brief Removes the node input of the
     * node inputs. Returns the inputNumber if it could remove it, otherwise returns
       -1.*/
    virtual int disconnectInput(Node* input);
    
    /**
     * @brief Same as:
      disconnectInput(inputNumber);
      connectInput(input,inputNumber);
     * Except that it is atomic
     **/
    bool replaceInput(const NodePtr& input,int inputNumber);
    
    void setNodeGuiPointer(const boost::shared_ptr<NodeGuiI>& gui);

    boost::shared_ptr<NodeGuiI> getNodeGui() const;
    
    bool isSettingsPanelOpened() const;
    
private:
    
    
    bool isSettingsPanelOpenedInternal(std::set<const Node*>& recursionList) const;
    
public:
    
    bool isUserSelected() const;
    
    bool shouldCacheOutput(bool isFrameVaryingOrAnimated, double time, ViewIdx view) const;

    /**
     * @brief If the session is a GUI session, then this function sets the position of the node on the nodegraph.
     **/
    void setPosition(double x, double y);
    void getPosition(double *x, double *y) const;
    
    void setSize(double w, double h);
    void getSize(double* w, double* h) const;
    
    /**
     * @brief Get the colour of the node as it appears on the nodegraph.
     **/
    void getColor(double* r,double *g, double* b) const;
    void setColor(double r, double g, double b);

    
    std::string getKnobChangedCallback() const;
    
    std::string getInputChangedCallback() const;

    /**
     * @brief This is used exclusively by nodes in the underlying graph of the implementation of the RotoPaint. 
     * Do not use that anywhere else.
     **/
    void attachRotoItem(const boost::shared_ptr<RotoDrawableItem>& stroke);
    boost::shared_ptr<RotoDrawableItem> getAttachedRotoItem() const;
    
    
    //This flag is used for the Roto plug-in and for the Merge inside the rotopaint tree
    //so that if the input of the roto node is RGB, it gets converted with alpha = 0, otherwise the user
    //won't be able to paint the alpha channel
    bool usesAlpha0ToConvertFromRGBToRGBA() const;
    void setUseAlpha0ToConvertFromRGBToRGBA(bool use);
    
protected:
    
    void runInputChangedCallback(int index);
private:
    
    /**
     * @brief Adds an output to this node.
     **/
    void connectOutput(bool useGuiValues, const NodePtr& output);

    /** @brief Removes the node output of the
     * node outputs. Returns the outputNumber if it could remove it,
       otherwise returns -1.*/
    int disconnectOutput(bool useGuiValues, const Node* output);
    
public:
    
    /**
     * @brief Switches the 2 first inputs that are not a mask, if and only if they have compatible components/bitdepths
     **/
    void switchInput0And1();
    /*============================*/

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginID() const;


    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginLabel() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginDescription() const;
    
    /**
     * @brief Returns the absolute file-path to the plug-in icon.
     **/
    std::string getPluginIconFilePath() const;

    /*============================*/
    AppInstance* getApp() const;

    /* @brief Make this node inactive. It will appear
       as if it was removed from the graph editor
       but the object still lives to allow
       undo/redo operations.
       @param outputsToDisconnect A list of the outputs whose inputs must be disconnected
       @param disconnectAll If set to true the parameter outputsToDisconnect is ignored and all outputs' inputs are disconnected
       @param reconnect If set to true Natron will attempt to re-connect disconnected output to an input of this node
       @param hideGui When true, the node gui will be notified so it gets hidden
     */
    void deactivate(const std::list< NodePtr > & outputsToDisconnect = std::list< NodePtr >(),
                    bool disconnectAll = true,
                    bool reconnect = true,
                    bool hideGui = true,
                    bool triggerRender = true);
    

    /* @brief Make this node active. It will appear
       again on the node graph.
       WARNING: this function can only be called
       after a call to deactivate() has been made.
     *
     * @param outputsToRestore Only the outputs specified that were previously connected to the node prior to the call to
     * deactivate() will be reconnected as output to this node.
     * @param restoreAll If true, the parameter outputsToRestore will be ignored.
     */
    void activate(const std::list< NodePtr > & outputsToRestore = std::list< NodePtr >(),
                  bool restoreAll = true,
                  bool triggerRender = true);
    
    /**
     * @brief Calls deactivate() and then remove the node from the project. The object will be destroyed
     * when the caller releases the reference to this Node
     * @param autoReconnect If set to true, outputs connected to this node will try to connect to the input of this node automatically.
     **/
    void destroyNode(bool autoReconnect);
    
private:
    
    void destroyNodeInternal(bool fromDest, bool autoReconnect);
    
public:
    

    /**
     * @brief Forwarded to the live effect instance
     **/
    KnobPtr getKnobByName(const std::string & name) const;

    /*@brief The derived class should query this to abort any long process
       in the engine function.*/
    bool aborted() const;

    /**
     * @brief Called externally when the rendering is aborted. You should never
     * call this yourself.
     **/
    void notifyRenderBeingAborted();

    bool makePreviewByDefault() const;

    void togglePreview();

    bool isPreviewEnabled() const;

    /**
     * @brief Makes a small 8bits preview image of size width x height of format ARGB32.
     * Pre-condition:
     *  - buf has been allocated for the correct amount of memory needed to fill the buffer.
     * Post-condition:
     *  - buf must not be freed or overflown.
     * It will serve as a preview on the node's graphical user interface.
     * This function is called directly by the GUI to display the preview.
     * In order to notify the GUI that you want to refresh the preview, just
     * call refreshPreviewImage(time).
     *
     * The width and height might be modified by the function, so their value can
     * be queried at the end of the function
     **/
    bool makePreviewImage(SequenceTime time, int *width, int *height, unsigned int* buf);

    /**
     * @brief Returns true if the node is currently rendering a preview image.
     **/
    bool isRenderingPreview() const;


    /**
     * @brief Returns true if the node is active for use in the graph editor. MT-safe
     **/
    bool isActivated() const;


    /**
     * @brief Use this function to post a transient message to the user. It will be displayed using
     * a dialog. The message can be of 4 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * eMessageTypeQuestion : you want to ask the user about something.
     * The function will return true always except for a message of type eMessageTypeQuestion, in which
     * case the function may return false if the user pressed the 'No' button.
     * @param content The message you want to pass.
     **/
    bool message(MessageTypeEnum type,const std::string & content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(MessageTypeEnum type,const std::string & content);

    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     * This function will also be called on all inputs
     **/
    void clearPersistentMessage(bool recurse);
    
private:
    
    void clearPersistentMessageRecursive(std::list<Node*>& markedNodes);
    
    void clearPersistentMessageInternal();
    
public:
    

    void purgeAllInstancesCaches();

    bool notifyInputNIsRendering(int inputNb);

    void notifyInputNIsFinishedRendering(int inputNb);

    bool notifyRenderingStarted();

    void notifyRenderingEnded();

    /**
     * @brief forwarded to the live instance
     **/
    void setOutputFilesForWriter(const std::string & pattern);


    ///called by EffectInstance
    void registerPluginMemory(size_t nBytes);

    ///called by EffectInstance
    void unregisterPluginMemory(size_t nBytes);

    //see eRenderSafetyInstanceSafe in EffectInstance::renderRoI
    //only 1 clone can render at any time
    QMutex & getRenderInstancesSharedMutex();

    void refreshPreviewsRecursivelyDownstream(double time);

    void refreshPreviewsRecursivelyUpstream(double time);

    void incrementKnobsAge();
    
    void incrementKnobsAge_internal();

    
    
public:
    
    

    U64 getKnobsAge() const;

    void onAllKnobsSlaved(bool isSlave,KnobHolder* master);

    void onKnobSlaved(KnobI* slave,KnobI* master,int dimension,bool isSlave);

    NodePtr getMasterNode() const;

    /**
     * @brief Attemps to lock an image for render. If it successfully obtained the lock,
     * the thread can continue and render normally. If another thread is currently
     * rendering that image, this function will wait until the image is available for render again.
     * This is used internally by EffectInstance::renderRoI
     **/
    void lock(const boost::shared_ptr<Image>& entry);
    bool tryLock(const boost::shared_ptr<Image>& entry);
    void unlock(const boost::shared_ptr<Image>& entry);


    /**
     * @brief DO NOT EVER USE THIS FUNCTION. This is provided for compatibility with plug-ins that
     * do not respect the OpenFX specification.
     **/
    boost::shared_ptr<Image> getImageBeingRendered(double time, unsigned int mipMapLevel, ViewIdx view);
    
    void beginInputEdition();
    
    void endInputEdition(bool triggerRender);

    void onInputChanged(int inputNb);

    void onEffectKnobValueChanged(KnobI* what,ValueChangedReasonEnum reason);

    bool isNodeDisabled() const;

    void setNodeDisabled(bool disabled);
    
    boost::shared_ptr<KnobBool> getDisabledKnob() const;

    void toggleBitDepthWarning(bool on,
                               const QString & tooltip)
    {
        Q_EMIT bitDepthWarningToggled(on, tooltip);
    }

    std::string getNodeExtraLabel() const;

    /**
     * @brief Show keyframe markers on the timeline. The signal to refresh the timeline's gui
     * will be emitted only if emitSignal is set to true.
     * Calling this function without calling hideKeyframesFromTimeline() has no effect.
     **/
    void showKeyframesOnTimeline(bool emitSignal);

    /**
     * @brief Hide keyframe markers on the timeline. The signal to refresh the timeline's gui
     * will be emitted only if emitSignal is set to true.
     * Calling this function without calling showKeyframesOnTimeline() has no effect.
     **/
    void hideKeyframesFromTimeline(bool emitSignal);

    bool areKeyframesVisibleOnTimeline() const;

    /**
     * @brief The given label is appended in the node's label but will not be editable
     * by the user from the settings panel.
     * If a custom data tag is found, it will replace any custom data.
     **/
    void replaceCustomDataInlabel(const QString & data);

    /**
     * @brief Returns whether this node or one of its inputs (recursively) is marked as
     * eSequentialPreferenceOnlySequential
     *
     * @param nodeName If the return value is true, this will be set to the name of the node
     * which is sequential.
     *
     **/
    bool hasSequentialOnlyNodeUpstream(std::string & nodeName) const;


    /**
     * @brief Updates the sub label knob: e.g for the Merge node it corresponds to the
     * operation name currently used and visible on the node
     **/
    void updateEffectLabelKnob(const QString & name);

    /**
     * @brief Returns true if an effect should be able to connect this node.
     **/
    bool canOthersConnectToThisNode() const;

    /**
     * @brief Clears any pointer refering to the last rendered image
     **/
    void clearLastRenderedImage();

    struct KnobLink
    {
        ///The knob being slaved
        KnobI* slave;
        KnobI* master;

        ///The master node to which the knob is slaved to
        NodeWPtr masterNode;

        ///The dimension being slaved, -1 if irrelevant
        int dimension;
    };

    void getKnobsLinks(std::list<KnobLink> & links) const;

    /*Initialises inputs*/
    void initializeInputs();

    /**
     * @brief Forwarded to the live effect instance
     **/
    void initializeKnobs(int renderScaleSupportPref, bool loadingSerialization);
    
    void checkForPremultWarningAndCheckboxes();
    
private:
    
    void initializeDefaultKnobs(int renderScaleSupportPref,bool loadingSerialization);
    
    void findPluginFormatKnobs(const KnobsVec & knobs,bool loadingSerialization);
    
    void createNodePage(const boost::shared_ptr<KnobPage>& settingsPage, int renderScaleSupportPref);
    
    void createInfoPage();
    
    void createPythonPage();
    
    void createHostMixKnob(const boost::shared_ptr<KnobPage>& mainPage);
    
    void createWriterFrameStepKnob(const boost::shared_ptr<KnobPage>& mainPage);
    
    void createMaskSelectors(const std::vector<std::pair<bool,bool> >& hasMaskChannelSelector,
                             const std::vector<std::string>& inputLabels,
                             const boost::shared_ptr<KnobPage>& mainPage,
                             bool addNewLineOnLastMask);
    
    boost::shared_ptr<KnobPage> getOrCreateMainPage();
    
    void createLabelKnob(const boost::shared_ptr<KnobPage>& settingsPage, const std::string& label);
    
    void findOrCreateChannelEnabled(const boost::shared_ptr<KnobPage>& mainPage);
    
    void createChannelSelectors(const std::vector<std::pair<bool,bool> >& hasMaskChannelSelector,
                                const std::vector<std::string>& inputLabels,
                                const boost::shared_ptr<KnobPage>& mainPage);
    
public:
    
    

    void onSetSupportRenderScaleMaybeSet(int support);
    
    bool useScaleOneImagesWhenRenderScaleSupportIsDisabled() const;
    
    /**
     * @brief Fills keyframes with all different keyframes time that all parameters of this
     * node have. Some keyframes might appear several times.
     **/
    void getAllKnobsKeyframes(std::list<SequenceTime>* keyframes);
    
    bool hasAnimatedKnob() const;
    
    
    void setNodeIsRendering();
    void unsetNodeIsRendering();
    
    /**
     * @brief Returns true if the parallel render args thread-storage is set
     **/
    bool isNodeRendering() const;
    
    bool hasPersistentMessage() const;
    
    void getPersistentMessage(QString* message,int* type, bool prefixLabelAndType = true) const;

    
    /**
     * @brief Attempts to detect cycles considering input being an input of this node.
     * Returns true if it couldn't detect any cycle, false otherwise.
     **/
    bool checkIfConnectingInputIsOk(Node* input) const;

    bool isForceCachingEnabled() const;
    
    
    /**
     * @brief Declares to Python all parameters as attribute of the variable representing this node.
     **/
    void declarePythonFields();
        
    /**
     * @brief Set the node name.
     * Throws a run-time error with the message in case of error
     **/
    void setScriptName(const std::string & name);

    void setScriptName_no_error_check(const std::string & name);
    
    
    /**
     * @brief The node unique name.
     **/
    const std::string & getScriptName() const;
    std::string getScriptName_mt_safe() const;
    
    /**
     @brief Returns the name of the node, prepended by the name of all the group containing it, e.g:
     * - a node in the "root" project would be: Blur1
     * - a node within the group 1 of the project would be : <g>group1</g>Blur1
     * - a node within the group 1 of the group 1 of the project would be : <g>group1</g><g>group1</g>Blur1
     **/
    std::string getFullyQualifiedName() const;

    void setLabel(const std::string& label);
    
    const std::string& getLabel() const;
    
    std::string getLabel_mt_safe() const;
    
    std::string getBeforeRenderCallback() const;
    std::string getBeforeFrameRenderCallback() const;
    std::string getAfterRenderCallback() const;
    std::string getAfterFrameRenderCallback() const;
    
    std::string getAfterNodeCreatedCallback() const;
    std::string getBeforeNodeRemovalCallback() const;
    
    void computeFrameRangeForReader(const KnobI* fileKnob);
    
    bool getOverlayColor(double* r,double* g,double* b) const;
    
    bool canHandleRenderScaleForOverlays() const;
    
    bool shouldDrawOverlay() const;
    
    
    void drawHostOverlay(double time, const RenderScale & renderScale);
    
    bool onOverlayPenDownDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;
    
    bool onOverlayPenMotionDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;
    
    bool onOverlayPenUpDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;
    
    bool onOverlayKeyDownDefault(const RenderScale & renderScale, Key key,KeyboardModifiers modifiers) WARN_UNUSED_RETURN;
    
    bool onOverlayKeyUpDefault(const RenderScale & renderScale, Key key,KeyboardModifiers modifiers) WARN_UNUSED_RETURN;
    
    bool onOverlayKeyRepeatDefault(const RenderScale & renderScale, Key key,KeyboardModifiers modifiers) WARN_UNUSED_RETURN;
    
    bool onOverlayFocusGainedDefault(const RenderScale & renderScale) WARN_UNUSED_RETURN;
    
    bool onOverlayFocusLostDefault(const RenderScale & renderScale) WARN_UNUSED_RETURN;
    
    void addDefaultPositionOverlay(const boost::shared_ptr<KnobDouble>& position);
    
    void addTransformInteract(const boost::shared_ptr<KnobDouble>& translate,
                              const boost::shared_ptr<KnobDouble>& scale,
                              const boost::shared_ptr<KnobBool>& scaleUniform,
                              const boost::shared_ptr<KnobDouble>& rotate,
                              const boost::shared_ptr<KnobDouble>& skewX,
                              const boost::shared_ptr<KnobDouble>& skewY,
                              const boost::shared_ptr<KnobChoice>& skewOrder,
                              const boost::shared_ptr<KnobDouble>& center);
    
    void removePositionHostOverlay(KnobI* knob);
    
    void initializeHostOverlays();
    
    bool hasHostOverlay() const;
    
    void setCurrentViewportForHostOverlays(OverlaySupport* viewPort);
    
    bool hasHostOverlayForParam(const KnobI* knob) const;

    void setPluginIconFilePath(const std::string& iconFilePath);
    
    void setPluginDescription(const std::string& description);
    
    void setPluginIDAndVersionForGui(const std::string& pluginLabel,const std::string& pluginID,unsigned int version);
    
    void setPluginPythonModule(const std::string& pythonModule);
    
    bool hasPyPlugBeenEdited() const;
    void setPyPlugEdited(bool edited);
    
    std::string getPluginPythonModule() const;
      
    //Returns true if changed
    bool refreshChannelSelectors();
    
    bool getProcessChannel(int channelIndex) const;
    
    boost::shared_ptr<KnobChoice> getChannelSelectorKnob(int inputNb) const;
    
    bool getSelectedLayer(int inputNb, std::bitset<4> *processChannels, bool* isAll, ImageComponents *layer) const;
    
    bool addUserComponents(const ImageComponents& comps);
    
    void getUserCreatedComponents(std::list<ImageComponents>* comps);
    
    bool hasAtLeastOneChannelToProcess() const;
    
    void removeParameterFromPython(const std::string& parameterName);

    double getHostMixingValue(double time, ViewIdx view) const;
    
    void removeAllImagesFromCacheWithMatchingIDAndDifferentKey(U64 nodeHashKey);
    void removeAllImagesFromCache(bool blocking);
    
    bool isDraftModeUsed() const;
    bool isInputRelatedDataDirty() const;
    
    void forceRefreshAllInputRelatedData();
    
    void markAllInputRelatedDataDirty();
    
    bool getSelectedLayerChoiceRaw(int inputNb,std::string& layer) const;
    
    const std::vector<std::string>& getCreatedViews() const;
    
    void refreshCreatedViews();
    
    void refreshIdentityState();
    
    bool getHideInputsKnobValue() const;
    void setHideInputsKnobValue(bool hidden);
    
    int getFrameStepKnobValue() const;
    
    void refreshFormatParamChoice(const std::vector<std::string>& entries, int defValue,bool canChangeValues);
    
    bool handleFormatKnob(KnobI* knob);
    
    QString makeHTMLDocumentation() const;
    
    
    
private:
    
    void computeHashRecursive(std::list<Node*>& marked);
    
    /**
     * @brief Refreshes the node hash depending on its context (knobs age, inputs etc...)
     * @return True if the hash has changed, false otherwise
     **/
    bool computeHashInternal() WARN_UNUSED_RETURN;
    
    void refreshEnabledKnobsLabel(const ImageComponents& layer);
    
    void refreshCreatedViews(KnobI* knob);
    
    void refreshInputRelatedDataRecursiveInternal(std::list<Node*>& markedNodes);
    
    void refreshInputRelatedDataRecursive();
    
    void refreshAllInputRelatedData(bool canChangeValues);
    
    bool refreshMaskEnabledNess(int inpubNb);
    
    bool refreshLayersChoiceSecretness(int inpubNb);
    
    void markInputRelatedDataDirtyRecursive();
    
    void markInputRelatedDataDirtyRecursiveInternal(std::list<Node*>& markedNodes,bool recurse);
    
    bool refreshAllInputRelatedData(bool hasSerializationData,const std::vector<NodeWPtr >& inputs);
    
    bool refreshInputRelatedDataInternal(std::list<Node*>& markedNodes);
    
    bool refreshDraftFlagInternal(const std::vector<NodeWPtr >& inputs);
    
    void setNameInternal(const std::string& name, bool throwErrors, bool declareToPython);
    
    std::string getFullyQualifiedNameInternal(const std::string& scriptName) const;
    
    void s_outputLayerChanged() { Q_EMIT outputLayerChanged(); }


    
public Q_SLOTS:

    void onRefreshIdentityStateRequestReceived();

    void setKnobsAge(U64 newAge);


    void doRefreshEdgesGUI()
    {
        Q_EMIT refreshEdgesGUI();
    }

    /*will force a preview re-computation not matter of the project's preview mode*/
    void computePreviewImage(double time)
    {
        Q_EMIT previewRefreshRequested(time);
    }

    /*will refresh the preview only if the project is in auto-preview mode*/
    void refreshPreviewImage(double time)
    {
        Q_EMIT previewImageChanged(time);
    }

    void onMasterNodeDeactivated();

    void onInputLabelChanged(const QString & name);

    void notifySettingsPanelClosed(bool closed )
    {
        Q_EMIT settingsPanelClosed(closed);
    }
    
    void dequeueActions();

    void onParentMultiInstanceInputChanged(int input);

    void doComputeHashOnMainThread();
    
Q_SIGNALS:
    
    void hideInputsKnobChanged(bool hidden);
    
    void refreshIdentityStateRequested();
    
    void availableViewsChanged();
    
    void outputLayerChanged();
    
    void mustComputeHashOnMainThread();

    void settingsPanelClosed(bool);

    void knobsAgeChanged(U64 age);

    void persistentMessageChanged();

    void inputsInitialized();

    void knobsInitialized();

    /*
     * @brief Emitted whenever an input changed on the GUI. Note that at the time this signal is emitted, the value returned by
     * getInput() is not necessarily the same as the value returned by getGuiInput() since the node might still be rendering.
     */
    void inputChanged(int);

    void outputsChanged();

    void activated(bool triggerRender);

    void deactivated(bool triggerRender);

    void canUndoChanged(bool);

    void canRedoChanged(bool);

    void labelChanged(QString);
    
    void scriptNameChanged(QString);
    
    void inputLabelChanged(int,QString);

    void refreshEdgesGUI();

    void previewImageChanged(double);

    void previewRefreshRequested(double);

    void inputNIsRendering(int inputNb);

    void inputNIsFinishedRendering(int inputNb);

    void renderingStarted();

    void renderingEnded();

    ///how much has just changed, this not the new value but the difference between the new value
    ///and the old value
    void pluginMemoryUsageChanged(qint64 mem);

    void allKnobsSlaved(bool b);

    ///Called when a knob is either slaved or unslaved
    void knobsLinksChanged();

    void knobSlaved();

    void previewKnobToggled();

    void disabledKnobToggled(bool disabled);

    void bitDepthWarningToggled(bool,QString);
    void nodeExtraLabelChanged(QString);

    
    void mustDequeueActions();
    
protected:

    /**
     * @brief Recompute the hash value of this node and notify all the clone effects that the values they store in their
     * knobs is dirty and that they should refresh it by cloning the live instance.
     **/
    void computeHash();

private:
    
    
    
    void declareRotoPythonField();

    std::string makeCacheInfo() const;
    std::string makeInfoForInput(int inputNumber) const;
    
    void setNodeIsRenderingInternal(std::list<Node*>& markedNodes);
    void setNodeIsNoLongerRenderingInternal(std::list<Node*>& markedNodes);
    



    /**
     * @brief If the node is an input of this node, set ok to true, otherwise
     * calls this function recursively on all inputs.
     **/
    void isNodeUpstream(const Node* input,bool* ok) const;
    
    void declareNodeVariableToPython(const std::string& nodeName);
    void setNodeVariableToPython(const std::string& oldName,const std::string& newName);
    void deleteNodeVariableToPython(const std::string& nodeName);

    boost::scoped_ptr<Implementation> _imp;
};

/**
 * @brief An InspectorNode is a type of node that is able to have a dynamic number of inputs.
 * Only 1 input is considered to be the "active" input of the InspectorNode, but several inputs
 * can be connected. This Node is suitable for effects that take only 1 input in parameter.
 * This is used for example by the Viewer, to be able to switch quickly from several inputs
 * while still having 1 input active.
 **/
class InspectorNode
    : public Node
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    

public:

    InspectorNode(AppInstance* app,
                  const boost::shared_ptr<NodeCollection>& group,
                  Plugin* plugin);

    virtual ~InspectorNode();


    /**
     * @brief Same as connectInputBase but if another input is already connected to 'input' then
     * it will disconnect it prior to connecting the input of the given number.
     **/
    virtual bool connectInput(const NodePtr& input,int inputNumber) OVERRIDE;

    virtual int getPreferredInputForConnection() const OVERRIDE FINAL;
    virtual int getPreferredInput() const OVERRIDE FINAL;

private:

    int getPreferredInputInternal(bool connected) const;

public:

    void setActiveInputAndRefresh(int inputNb, bool fromViewer);

};


class RenderingFlagSetter
{
    Node* node;
public:
    
    RenderingFlagSetter(Node* n)
    : node(n)
    {
        node->setNodeIsRendering();
    }
    
    ~RenderingFlagSetter()
    {
        node->unsetNodeIsRendering();
    }
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_NODE_H
