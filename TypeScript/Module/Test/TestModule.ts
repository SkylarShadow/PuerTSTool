import ModuleBase from "../../Framework/Module/ModuleBase";

class TestModule extends ModuleBase {
    constructor() {
        super();
        // Additional initialization if needed
    }

    Initialize(): void {
        super.Initialize();
        // Implement your own initialization logic here
        console.log("TestModule initialized");
    }

    Start(): void {
        super.Start();
        // Implement your own start logic here
        console.log("TestModule started");

    }
}

export default TestModule;