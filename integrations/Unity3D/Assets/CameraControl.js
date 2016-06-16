
var zoomSpeed : float = 3.0f;
var moveSpeed : float = 3.0f;
var rotateSpeed : float = 6.0f;

var optionalMaterialForSelection : Material;

// Some internal placeholders
private var orbitVector : GameObject;
private var materialForSelection : Material;
private var selectedObjects = new ArrayList();
private var selectedObjectsMaterial = new ArrayList();

function Start() {
    // Create a capsule (which will be the lookAt target and global orbit vector)
    orbitVector = GameObject.CreatePrimitive(PrimitiveType.Capsule);
    orbitVector.transform.position = Vector3.zero;
    // Snap the camera to align with the grid in set starting position (otherwise everything gets a bit wonky)
    transform.position = Vector3(4, 4, 4);
    // Point the camera towards the capsule
    transform.LookAt(orbitVector.transform.position, Vector3.up);
    // Hide the capsule (disable the mesh renderer)
    orbitVector.GetComponent.<Renderer>().enabled = false;
    // Create material to apply for selections (or use material supplied by user)
    if (optionalMaterialForSelection) {
        materialForSelection = optionalMaterialForSelection;
    } else {
        materialForSelection = new Material(Shader.Find("Diffuse"));
        materialForSelection.color = Color.green;
    }
}

function Update(){

orbitVector.transform.position = Vector3.zero;

	if (Input.GetAxis("Mouse ScrollWheel") < 0) {
	    var currentZoomSpeedin = -10;
	    transform.Translate(Vector3.forward * ( currentZoomSpeedin));
	}
	if (Input.GetAxis("Mouse ScrollWheel") > 0) {
	    var currentZoomSpeedout = 10;
	    transform.Translate(Vector3.forward * ( currentZoomSpeedout));
	}
}

// Call all of our functionality in LateUpdate() to avoid weird behaviour (as seen in Update())
function LateUpdate() {
    // Get mouse vectors
    var x = Input.GetAxis("Mouse X");
    var y = Input.GetAxis("Mouse Y");
    
    // ALT is pressed, start navigation
    //if (Input.GetKey(KeyCode.RightAlt) || Input.GetKey(KeyCode.LeftAlt)) {
    
        // Distance between camera and orbitVector. We'll need this in a few places
        var distanceToOrbit = Vector3.Distance(transform.position, orbitVector.transform.position);
    
        //RMB - ZOOM
        if (Input.GetMouseButton(2)) {
            
            // Refine the rotateSpeed based on distance to orbitVector
            var currentZoomSpeed = Mathf.Clamp(zoomSpeed * (distanceToOrbit / 50), 0.1f, 2.0f);
            
            // Move the camera in/out
            transform.Translate(Vector3.forward * (x * currentZoomSpeed));
            
            // If about to collide with the orbitVector, repulse the orbitVector slightly to keep it in front of us
            if (Vector3.Distance(transform.position, orbitVector.transform.position) < 3) {
                orbitVector.transform.Translate(Vector3.forward, transform);
            }

        
        //LMB - PIVOT
        } else if (Input.GetMouseButton(1)) {
            
            // Refine the rotateSpeed based on distance to orbitVector
            var currentRotateSpeed = Mathf.Clamp(rotateSpeed * (distanceToOrbit / 50), 1.0f, rotateSpeed);
            
            // Temporarily parent the camera to orbitVector and rotate orbitVector as desired
            transform.parent = orbitVector.transform;
            orbitVector.transform.Rotate(Vector3.right * (y * currentRotateSpeed));
            orbitVector.transform.Rotate(Vector3.up * (x * currentRotateSpeed), Space.World);
            transform.parent = null;
            
            
        //MMB - PAN
        } else if (Input.GetMouseButton(0)) {
            
            // Calculate move speed
            var translateX = Vector3.right * (x * moveSpeed) * -1;
            var translateY = Vector3.up * (y * moveSpeed) * -1;
            
            // Move the camera
            transform.Translate(translateX);
            transform.Translate(translateY);
            
            // Move the orbitVector with the same values, along the camera's axes. In effect causing it to behave as if temporarily parented.
            orbitVector.transform.Translate(translateX, transform);
            orbitVector.transform.Translate(translateY, transform);
        }
        
        
     /*   
    // If we're not currently navigating, grab selection if something is clicked
    } else if (Input.GetMouseButtonDown(0)) {
        var hitInfo : RaycastHit;
        var ray : Ray = camera.ScreenPointToRay(Input.mousePosition);
        var allowMultiSelect : boolean = false;
        
        // See if the user is holding in CTRL or SHIFT. If so, enable multiselection
        if(Input.GetKey(KeyCode.RightShift) || Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightControl) || Input.GetKey(KeyCode.LeftControl)) {
            allowMultiSelect = true;
        }
        
        // Something was clicked. Fetch.
        if (Physics.Raycast(ray, hitInfo, camera.farClipPlane)) {
            target = hitInfo.transform;
            
            // If NOT multiselection, remove all prior selections
            if (!allowMultiSelect) {
                deselectAll();
            }
            
            //Toggle between selected and unselected (depending on current state)
            if (target.renderer.sharedMaterial != materialForSelection) {
                selectedObjects.Add(target.gameObject);
                selectedObjectsMaterial.Add(target.gameObject.renderer.sharedMaterial);
                target.gameObject.renderer.sharedMaterial = materialForSelection;
            
            } else {
                var arrayLocation : int = selectedObjects.IndexOf(target.gameObject);
                if (arrayLocation == -1) {return;}; //this shouldn't happen. Ever. But still.
                
                target.gameObject.renderer.sharedMaterial = selectedObjectsMaterial[arrayLocation];
                selectedObjects.RemoveAt(arrayLocation);
                selectedObjectsMaterial.RemoveAt(arrayLocation);
                
            }
            
        // Else deselect all selected objects (ie. click on empty background)
        } else {
            
            // Don't deselect if allowMultiSelect is true
            if (!allowMultiSelect) {deselectAll();};
        }
        
        
        
    // Fetch input of the F-button (focus) -- this is a very dodgy implementation...
    } else if (Input.GetKeyDown("f")) {
        var backtrack = Vector3(0, 0, -15);
        var selectedObject : GameObject;
        
        // If dealing with only one selected object
        if (selectedObjects.Count == 1) {
            selectedObject = selectedObjects[0];
            transform.position = selectedObject.transform.position;
            orbitVector.transform.position = selectedObject.transform.position;
            transform.Translate(backtrack);
        
        // Else we need to average out the position vectors (this is the proper dodgy part of the implementation)
        } else if (selectedObjects.Count > 1) {
            selectedObject = selectedObjects[0];
            var average = selectedObject.transform.position;
        
            for (var i = 1; i < selectedObjects.Count; i++) {
                selectedObject = selectedObjects[i];
                average = (average + selectedObject.transform.position) / 2;
            }
            
            transform.position = average;
            orbitVector.transform.position = average;
            transform.Translate(backtrack);
        }
    }
    */
}



// Function to handle the de-selection of all objects in scene
function deselectAll() {

    // Run through the list of selected objects and restore their original materials
    for (var currentItem = 0; currentItem < selectedObjects.Count; currentItem++) {
        var selectedObject : GameObject = selectedObjects[currentItem];
        selectedObject.GetComponent.<Renderer>().sharedMaterial = selectedObjectsMaterial[currentItem];
    }
    
    // Clear both arrays
    selectedObjects.Clear();
    selectedObjectsMaterial.Clear();
}