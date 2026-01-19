import EventDefine from "../../Define/EventDefine";
import EventDispatcher from "../Utils/EventDispatcher";
import * as UE from 'ue';
import UIManager from "./UIManager";

export default abstract class UIBase {

    protected m_Event: EventDispatcher;

    Construct(): void {
        if (this.m_Event == null) {
            this.m_Event = new EventDispatcher();
            Object.hasOwn(this, "Event") || Object.defineProperty(this, "Event", {
                get: () => {
                    return this.m_Event;
                }
            });
        }

        this.m_Event.DispatchEvent(EventDefine.UI_CREATE, this);
        UIManager.GetInstance().CacheUI(this as any as UE.UserWidget);
    }

    Destruct(): void {
        this.m_Event.DispatchEvent(EventDefine.UI_DESTROY, this);
        this.m_Event.RemoveAllEventListeners();
        this.m_Event = null;
    }

    public get Event(): EventDispatcher {
        return this.m_Event;
    }


    /*
         *Adds the widget to the game's viewport in a section dedicated to the player.  This is valuable in a split screen
         *game where you need to only show a widget over a player's portion of the viewport.
         *
         *@param ZOrder The higher the number, the more on top this widget will be.
         */
    AddToPlayerScreen(ZOrder?: number /* = 0 */): boolean { console.error("函数重载失效！"); return false; };
    /*
     *Adds it to the game's viewport and fills the entire screen, unless SetDesiredSizeInViewport is called
     *to explicitly set the size.
     *
     *@param ZOrder The higher the number, the more on top this widget will be.
     */
    AddToViewport(ZOrder?: number /* = 0 */): void { console.error("函数重载失效！"); };

    RemoveFromViewport(): void { console.error("函数重载失效！"); };

    /*
    *Removes the widget from its parent widget.  If this widget was added to the player's screen or the viewport
    *it will also be removed from those containers.
    */
    RemoveFromParent(): void { console.error("函数重载失效！"); };

    /*
             *Sets the visibility of the widget.
             */
    SetVisibility(InVisibility: UE.ESlateVisibility): void { console.error("函数重载失效！"); };

}