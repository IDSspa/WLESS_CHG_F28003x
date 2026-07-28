function [Ptrasf_loc_rif_lim, I_coil_loc_err, duty_cycle_PWM_A, duty_cycle_PWM_B, abilita_PWM, duty_cycle_PS_A, duty_cycle_PS_B, abilita_PS, Vac_rif, Stato_loc]=...
                            Controllo_Sistema_WPT(I_coil_loc, I_coil_rem_err, V_dc_Pbat_rif, V_dc_Ptrasf_rif, V_dc, V_bat, I_L_A, I_L_B, I_bat_rif_max, I_bat_rif_min, Ptrasf_rem_rif_lim, Stato_rem, Comando_Attivazione, Param_Contr)

% Questa funzione implementa:
% 1) Il controllo della corrente scambiata tra la batteria e il condensatore 
% del bus dc tramite il convertitore dc-dc. Questo controllo è sempre
% attivo.
% 2) La regolazione della tensione del bus dc utilizzando l'anello di corrente 
% implementato al punto 1) Questo controllo è sempre attivo.
% 3) Il controllo della corrente nella bobina remota (quella dell'altra sezione del sistema)
% mediante la manipolazione della tensione generata dal convertitore dc-ac
% locale (di questa sezione del convertitore). Questo anello di corrente
% sarà sfruttato come anello interno da un anello di regolazione della
% tensione del bus dc. Questo controllo è attivo solo quando si deve
% trasmettere potenza all'altra sezione
% 4) Il controllo della tensione del bus dc locale mediante la potenza
% ricevuta dalla sezione remota. Il controllore di questo anello genera un
% riferimento per la potenza da ricevere. E' attivo quando si deve ricevere
% potenza dall'altra sezione sezione (si deve caricare la batteria).
% 5) Il controllo della tensione del bus dc locale mediante potenza da
% trasmettere alla sezione remota del sistema. Il controllore di questo
% anello genera un riferimento di potenza da trasmettere. E' attivo quando
% si deve trasmettere potenza all'altra sezione (si deve scaricare la batteria).
% Il riferimento di potenza viene trasmesso via radio alla sezione remota
% dove viene confrontato con quello generato dall'anello di controllo 4). Il
% più piccolo dei due riferimento viene usato per calcolare un riferimento
% di ampiezza per la corrente della bobina ricevente. Il riferimento di
% corrente viene inviato via radio alla sezione trasmittente dove viene
% implementato l'anello di controllo 3) 

% Nella realizzazione in C questo codice è parte di una routine di servizio
% di un interrupt per cui non ci saranno variabili di ingresso e uscita e la
% funzione dovrà operare su variabili condivise con le altre sezioni del
% firmware.
% Tutte le grandezze sono supposte di tipo float e sono scalate nelle unità
% naturali (Volt, Ampere,...).
% Nel prototipo realizzato a Padova la routine dei servizio veniva eseguita
% come risposta ad un interrupt generato alla fine della conversione
% analogico-digitale delle varie grandezze di interesse. Di conseguenza
% quando la routine viene eseguita tutti i risultati della conversione sono
% già disponibili. A sua volta, la conversione analogico-digitale è
% effettuata all'inizio del periodo di PWM.

% Al fine di garantire un tempo di elaborazione sufficiente alla routine di
% servizio si è supposto che essa venga eseguita con una frequenza pari a
% 1/4 di quella di alimentazione delle bobine. (85 kHz/4 = 21.25 kHz).
% Quindi il periodo di campionamento risulta di circa 47 us.
% La medesima routine di servizio deve gestire sia il convertitore dc-dc
% che il convertitore dc-ac. La frequenza di switching del primo era stata
% fissata a 120 kHz come nel prototipo Texas. questa frequenza non è multipla
% di quella di campionamento per cui si propone di alzare la frequenza di
% switching del convertitore dc-dc a 6 x 21.25 kHz = 127.5 kHz. In questo
% modo non sarà necessario ridimensionare le induttanze di filtro collegate
% all'ingresso del convertitore dc-dc.

% Variabili in ingresso
%
% I_coil_loc è il valore attuale dell'ampiezza della corrente nella bobina locale.
% Viene acquisito tramite i convertitori A/D a partire dal segnale generato
% dal circuito che trasduce l'ampiezza della corrente.
% La differenza
%
% I_coil_loc_err=I_coil_loc_rif-I_coil_loc
%
% viene inviata tramite radio all'altra sezione del sistema.
%
% I_coil_rem_err è l'errore di ampiezza di corrente nella bobina remota
% (dell'altra sezione del sistema). E' ricevuto via radio quando la sezione
% locale trasferisce potenza alla sezione remota. Altrimenti viene ricevuto
% Ptrasf_rem_rif_lim.
% In base a I_coil_rem_err viene calcolata la tensione generata dal convertitore
% dc-ac locale per controllare l'ampiezza di corrente della bobina remota.
% 
% % V_dc_rif, V_dc, sono i valori di riferimento e il valore attuale della
% tensione del bus dc. Durante il funzionamento V_dc_rif è costante e assume
% due valori diversi a seconda che la batteria venga caricata o scaricata.
% 
% V_bat è il valore attuale della tensione di batteria. E' utilizzato per calcolare
% la potenza massima che può essere scambiata con la batteria e il valore dei
% duty cycle da applicare alle due gambe del convertitore dc-dc.
% 
% I_L_A, I_L_B, sono i valori attuali delle correnti nei due rami del
% convertitore dc-dc 
%
% I_bat_rif_max, I_bat_rif_min, sono il limite superiore (durante la scarica)
% e inferiore (durante la carica) della corrente che può essere estratta o
% iniettata nella batteria. Sono forniti dal BMS.
%
% Ptrasf_rem_rif_lim è il limite di potenza trasferibile calcolato della
% sezione remota. Viene ricevuto via radio quando la sezione locale riceve 
% potenza dalla sezione remota. Altrimenti viene ricevuto I_coil_rem_err.
%
% Stato_rem è lo stato della sezione remota. Nel seguito del listato sono elencati
% gli stati possibili per la sezione "source" e la sezione "load".
%
% Comando_Attivazione può assumere 3 valori: 0 abilita la ricezione della
% potenza e quindi impone alla sezione locale di operare come "load".
% 1 abilita la trasmissione dipotenza e quindi impone alla sezione locale
% di operare come "source".
% 2 Annulla lo scambio di potenza e riporta la sezione locare nello stato
% di partenza, che è uguale per le due sezioni.
%
% Param_Contr sono i parametri usati dai controllori (Kp, Ki, limiti...)
% I parametri sono calcolati dal file p_schema_completo_generale_v2.m

% Variabili in uscita
%
% Ptrasf_loc_rif_lim è il limite della potenza trasferibile. Viene trasmesso
% via radio alla sezione remota quando la sezione locale traferisce potenza
% alla sezione remota. Altrimenti viene trasmesso via radio I_coil_loc_err   
%
% I_coil_loc_err è l'errore locale di corrente. Viene trasmesso via radio
% alla sezione remota el sistema quando la sezione locale riceve la potenza dalla 
% sezione remota. Altrimenti viene trasmesso via radio Ptrasf_loc_rif_lim
%
% duty_cycle_PWM_A, duty_cycle_B sono i duty cycle per i due rami del
% convertitore buck-boost dc-dc.  I duty cycle sono compresi tra 0 e 1.
% Vanno scalati opportunamente per essere impiegati nella periferica del
% microcontrollore che genera i comandi di PWM.
%
% abilita_PWM viene utilizzato per abilitare il convertitore dc-dc quando viene
% effettuato uno scambio di potenza tra le due sezioni del sistema.
% L'abilitazione viene leggermente ritardata rispetto all'accensione del sistema
% WPT per permettere ai vari filtri che processano i segnali analogici di
% raggiungere la condizione di regime.
%
% duty_cycle_PS_A, duty_cycle_PS_B, sono i duty cycle per i due rami del
% convertitore dc-ac.  I duty cycle sono compresi tra 0 e 1 e sono legati
% allo sfasamento tra le due tensioni ad onda quadra generate sui punti centrali
% delle gambe del convertitore ac-dc. Maggiori dettagli si trovano nella
% relazione "Controllo del convertitore dc-ac"
%
% abilita_PS viene utilizzato per abilitare il convertitore dc-ac quando deve
% essere trasmessa potenza all'altra sezione del sistema.
% L'eventuale abilitazione viene leggermente ritardata rispetto all'accensione
% del sistema WPT per permettere ai vari filtri che processano i segnali
% analogici di raggiungere la condizione di regime.
%
% Stato_loc è lo stato della sezione locale. Nel seguito del listatato sono
% elencati gli stati possibili per la sezione "source" e la sezione "load"



V_dc_ref_low=245; % Soglia per fine precarica del condensatore del bus dc

Icoil_amp_max=40;% Massima ampiezza di corrente nella bobina. Serve a limitare il riferimento da cui si calcola I_coil_loc_err
Icoil_amp_min=0; % Minima ampiezza di corrente nella bobina
I_coil_loc_min=0.1; % Minima ampiezza della corrente nella bobina trasmittente per spegnere il convertitore dc/ac

% Limiti minimi di corrente e potenza.
% Servono a decidere quando considerare completato il trasferimento di potenza
I_bat_rif_tx_lim=1;
I_bat_rif_rx_lim=1;
I_L_lim=0.25;
%P_trasf_rem_rif_lim=5;

% Ritardo abilitazione PWM
% E' il tempo di attesa affinchè i trasduttori si portino a regime dopo
% l'accensione del sistema
Durata_Attesa_Trasduttori=100; % Misurato in periodi di campionamento


% Possibili valori di "Comando_Attivazione"
Annulla_Scambio_Potenza=2;
Abilita_Trasmissione_Potenza=1;
Abilita_Ricezione_Potenza=0;


% Stato di partenza
Attesa_Attivazione=0;

% Stati possibili per la sezione "source"
Attesa_Regime_Trasduttori_tx=1;
Attesa_Contatto_Radio_tx=2;
Precarica_Bus_dc_tx=3;
Trasferimento_tx=4;
Fine_Trasferimento_dc_dc_tx=5;
Fine_Trasferimento_dc_ac_tx=6;
Spento_tx=7;

% Stati possibili per la sezione "load"
Attesa_Regime_Trasduttori_rx=1.1;
Attesa_Trasferimento_rx=2.1;
Precarica_Bus_dc_rx=3.1;
Trasferimento_rx=4.1;
Richiesta_Fine_Trasferimento_rx=5.1;
Spento_rx=7.1;


% Variabili statiche.
% Devono essere mantenute tra una chiamata e l'altra della routine di servizio.
% Sono equivalenti alle variabili "static" in C
 persistent Inizializzato
 persistent Contatore
 persistent Stato


% Inizializzazione delle variabili persistent
% In C corrisponde ad assegnare il valore iniziale alla variabili statiche
% al momento della dichiarazione, per cui non serve una struttura "if"
if isempty(Inizializzato)
    Inizializzato = 1;
    Contatore = 0;
    Stato=Attesa_Attivazione;
end

% Inizializzo le variabili di uscita come richiesto da Simulink
Ptrasf_loc_rif_lim=0;
I_coil_loc_err=0;
duty_cycle_PWM_A=0.5;
duty_cycle_PWM_B=0.5;
abilita_PWM=0;
duty_cycle_PS_A=0.5;
duty_cycle_PS_B=0.5;
abilita_PS=0;
Stato_loc=Attesa_Attivazione;

Vac_rif=0;

switch(Stato)
    case Attesa_Attivazione
        switch(Comando_Attivazione)
            case Annulla_Scambio_Potenza
                Stato=Attesa_Attivazione;

            case Abilita_Trasmissione_Potenza
                Stato=Attesa_Regime_Trasduttori_tx;

            case Abilita_Ricezione_Potenza
                Stato=Attesa_Regime_Trasduttori_rx;
        end
        Stato_loc=Stato;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                         Inizio anelli di controllo                                 %
%                                trasmettitore                                       %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    case Attesa_Regime_Trasduttori_tx % Inserisce un piccolo ritardo all'accensione prima di iniziare a controllare 
                                      % i convertitore in modo che i filtri e i trasduttori della varie
                                      % grandezza vadano a regime

        abilita_PWM=0;          % Disabilita il convertitore dc-dc
        abilita_PS=0;           % Disabilita il convertitore dc-ac
        Ptrasf_loc_rif_lim=0;   % Gli anelli di controllo non sono ancora attivi.
                                % Questo valore al momento non è utilizzato
        Stato_loc=Stato;

        Contatore=Contatore+1;
        if (Contatore>=Durata_Attesa_Trasduttori)
            Stato=Attesa_Contatto_Radio_tx;
        end
% Se il trasferimento di potenza viene disabilitato ripristino le condizioni di partenza       
        if(Comando_Attivazione~=Abilita_Trasmissione_Potenza)
            Stato=Spento_tx;
        end 


    case Attesa_Contatto_Radio_tx % Dopo che i trasduttori sono a regime aspetta di ricevere un messaggio dalla sezione ricevente
                                  % per avere conferma che è presente e pronta ad effettuare la ricarica della  batteria
                               
        abilita_PWM=0; % Disabilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac
        Ptrasf_loc_rif_lim=0; % Rende disponibile lo stato alla sezione ricevente

        Stato_loc=Stato;

        if(Stato_rem==Attesa_Trasferimento_rx) % La sezione ricevente è presente e pronta
            Stato=Precarica_Bus_dc_tx;
        end

% Se il trasferimento di potenza viene disabilitato ripristino le condizioni di partenza
% Non c'è la procedura di spegnimento perchè i convertitori non sono ancora
% stati accesi
        if(Comando_Attivazione~=Abilita_Trasmissione_Potenza)
            Stato=Spento_tx;
        end    


    case Precarica_Bus_dc_tx
        abilita_PWM=1; % Abilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac
        Ptrasf_loc_rif_lim=0; 
        Stato_loc=Stato;

% Controllo della tensione del bus dc mediante scambio di potenza con la batteria
% Viene generato un riferimento di corrente di batteria
        [I_bat_rif] = Controllo_Tensione_Bus_dc_P_bat(V_dc_Pbat_rif, V_dc, V_bat, I_bat_rif_max, I_bat_rif_min, Param_Contr, 1);            
% Controllo della corrente di batteria            
        [duty_cycle_PWM_A, duty_cycle_PWM_B] = Controllo_Corrente_Batteria(I_bat_rif, I_L_A, I_L_B, V_dc, V_bat, Param_Contr, 1);
        
% Se ha raggiunto la soglia inferiore per la tensione del bus dc inizia a
% trasmettere potenza all'altra sezione.
        if(V_dc>=V_dc_ref_low)
            Stato=Trasferimento_tx;
        end

        if(I_bat_rif_max<I_bat_rif_tx_lim) % Se si verifica questa condizione la batteria "source" è scarica
            Stato=Fine_Trasferimento_dc_dc_tx; % quindi si disabilita la trasmissione di potenza
        end 

% Se la sezione ricevente non è più in attesa del trasferimento disabilita
% la trasmissione di potenza
        if(Stato_rem~=Attesa_Trasferimento_rx)
            Stato=Fine_Trasferimento_dc_dc_tx;
        end

% Se il trasferimento di potenza viene disabilitato va alla procedura di spegnimento       
        if(Comando_Attivazione~=Abilita_Trasmissione_Potenza)
            Stato=Fine_Trasferimento_dc_dc_tx;
        end 


    case Trasferimento_tx
        abilita_PWM=1; % Abilita il convertitore dc-dc
        abilita_PS=1;  % Abilita il convertitore dc-ac

        Stato_loc=Stato;



% Controllo della tensione del bus dc mediante scambio di potenza con la batteria
% Viene generato un riferimento di corrente di batteria
        [I_bat_rif] = Controllo_Tensione_Bus_dc_P_bat(V_dc_Pbat_rif, V_dc, V_bat, I_bat_rif_max, I_bat_rif_min,Param_Contr, 1);         
% Controllo della corrente di batteria            
        [duty_cycle_PWM_A, duty_cycle_PWM_B] = Controllo_Corrente_Batteria(I_bat_rif, I_L_A, I_L_B, V_dc, V_bat,  Param_Contr, 1);

% Controllo della tensione del bus dc mediante scambio di potenza con la sezione ricevente
% Viene generato un riferimento per la potenza trasmessa. Questo riferimento è inviato via radio
% alla sezione ricevente che lo usa come limite superiore per la potenza da ricevere
% (se la potenza ricevuta fossa maggiore il condensatore della sezione trasmittente si scaricherebbe)
        [P_trasf_rem_rif]=Controllo_Tensione_Bus_dc_P_trasf(V_dc, V_dc_Ptrasf_rif, Icoil_amp_max, Icoil_amp_min, Param_Contr, 1);
        Ptrasf_loc_rif_lim=P_trasf_rem_rif; % Rende disponibile via radio il riferimento alla sezione "load"

% Controlla la corrente nella bobina ricevente mediante la tensione generata dal convertitore dc-ac.
% L'errore di corrente è inviato via radio dalla sezione "load"
        [duty_cycle_PS_A, duty_cycle_PS_B, Vac_rif]=Controllo_Corrente_Bobina_Remota_e_Limitazione_Corrente_Bobina_Locale(V_dc, I_coil_rem_err, I_coil_loc, Param_Contr, 1);

        
        if(I_bat_rif_max<I_bat_rif_tx_lim) % Se si verifica questa condizione la batteria "source" è scarica
            Stato=Fine_Trasferimento_dc_dc_tx;
        end 
        

        % if((Stato_rem~=Attesa_Trasferimento_rx)&&(Stato_rem~=Precarica_Bus_dc_rx)&&(Stato_rem~=Trasferimento_rx))
        %     Stato=Fine_Trasferimento_dc_dc_tx;
        % end

        if((Stato_rem==Richiesta_Fine_Trasferimento_rx)||(Stato_rem==Spento_rx)||(Stato_rem==Attesa_Attivazione))
            Stato=Fine_Trasferimento_dc_dc_tx;
        end


        % Se il trasferimento di potenza viene disabilitato va alla procedura di spegnimento       
        if(Comando_Attivazione~=Abilita_Trasmissione_Potenza)
            Stato=Fine_Trasferimento_dc_dc_tx;
        end 
  


    case Fine_Trasferimento_dc_dc_tx % Disabilita il il convertitore dc-dc e usa il convertitore
        %  dc-ac per scaricare il condensatore
        abilita_PWM=0; % Disabilita il convertitore dc-dc. La corrente di batteria si annulla da sola
                       % Il condensatore del bus dc si scarica perchè sta ancora trasferendo  potenza
        abilita_PS=1;  % Abilita il convertitore dc-ac per continaure a controllare la tnsione del condensatore

        Stato_loc=Stato;

% Controllo della tensione del bus dc mediante scambio di potenza con la sezione ricevente
% Viene generato un riferimento per la potenza trasmessa. Questo riferimento è inviato via radio
% alla sezione ricevente che lo usa come limite superiore per la potenza da ricevere
% (se la potenza ricevuta fossa maggiore il condensatore della sezione trasmittente si scaricherebbe)
        [P_trasf_rem_rif]=Controllo_Tensione_Bus_dc_P_trasf(V_dc, V_dc_Ptrasf_rif, Icoil_amp_max, Icoil_amp_min, Param_Contr, 1);
        Ptrasf_loc_rif_lim=P_trasf_rem_rif; % Rende disponibile il riferimento alla sezione ricevente

% Controlla la corrente nella bobina ricevente mediante la tensione generata dal convertitore dc-ac.
% L'errore di corrente è ricevuto via radio

        [duty_cycle_PS_A, duty_cycle_PS_B, Vac_rif]=Controllo_Corrente_Bobina_Remota_e_Limitazione_Corrente_Bobina_Locale(V_dc, I_coil_rem_err, I_coil_loc, Param_Contr, 1);
 
% Aspetta che la corrente nella batteria "source" si riduca al valore limite di spegnimento       
        if((abs(I_L_A)<I_L_lim)&&(abs(I_L_B)<I_L_lim))
            Stato=Spento_tx;
            Stato=Fine_Trasferimento_dc_ac_tx;
        end

    case Fine_Trasferimento_dc_ac_tx
        abilita_PWM=0; % Disbilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac
        Stato_loc=Stato;

% Azzero la potenza trasferita. Questo forza la corrente nella bobina
% ricevente ad azzerarsi
% Viene generato un riferimento per la potenza trasmessa. Questo riferimento è inviato via radio
% alla sezione ricevente che lo usa come limite superiore per la potenza da ricevere
% (se la potenza ricevuta fossa maggiore il condensatore della sezione trasmittente si scaricherebbe)
%        [P_trasf_rem_rif]=Controllo_Tensione_Bus_dc_P_trasf(V_dc, V_dc_Ptrasf_rif, Icoil_amp_max, Icoil_amp_min, Param_Contr, 1);
%        Ptrasf_loc_rif_lim=0; % Rende disponibile il riferimento alla sezione ricevente

% Controlla la corrente nella bobina ricevente mediante la tensione generata dal convertitore dc-ac.
% L'errore di corrente è ricevuto via radio
%        [duty_cycle_PS_A, duty_cycle_PS_B, Vac_rif]=Controllo_Corrente_Bobina_Remota_e_Limitazione_Corrente_Bobina_Locale(V_dc, I_coil_rem_err, I_coil_loc, Param_Contr, 1);


% Aspetta che la corrente nella bobina "source" si annulli
        if(I_coil_loc<I_coil_loc_min)
            Stato=Spento_tx;
        end

    
    case Spento_tx
        abilita_PWM=0; % Disbilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac
        Stato_loc=Stato;
        Ptrasf_loc_rif_lim=0;

% Aspetta che il comando di trasferimento sia disabilitato prima di ripristinare
% le condizioni di partenza
        if(Comando_Attivazione==Annulla_Scambio_Potenza)
            Stato=Attesa_Attivazione;
% Inizializza lo stato della funzioni di controllo
            Controllo_Tensione_Bus_dc_P_bat(0, 0, 0, 0, 0, 0, 0);            
            Controllo_Corrente_Batteria(0, 0, 0, 0, 0, 0, 0);
            Controllo_Tensione_Bus_dc_P_trasf(0, 0, 0, 0, 0, 0);
            Controllo_Corrente_Bobina_Remota_e_Limitazione_Corrente_Bobina_Locale(0, 0, 0, 0, 0);
            Contatore=0;
        end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                           Fine anelli di controllo                                 %
%                                trasmettitore                                       %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                         Inizio anelli di controllo                                 %
%                                 ricevitore                                         %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    


    case Attesa_Regime_Trasduttori_rx % Inserisce un piccolo ritardo all'accensione prima di iniziare a controllare 
                                      % i convertitore in modo che i filtri e i trasduttori della varie
                                      % grandezza vadano a regime

        abilita_PWM=0;          % Disabilita il convertitore dc-dc
        abilita_PS=0;           % Disabilita il convertitore dc-ac
        I_coil_loc_err=0;       % Gli anelli di controllo non sono ancora attivi.
                                % Questo valore al momento non è utilizzato
        Stato_loc=Stato;

        Contatore=Contatore+1;
        if (Contatore>=Durata_Attesa_Trasduttori)
            Stato=Attesa_Trasferimento_rx;
        end

% Se il trasferimento di potenza viene disabilitato ripristino le condizioni di partenza       
        if(Comando_Attivazione~=Abilita_Ricezione_Potenza)
            Stato=Spento_rx;
        end 

    
    case Attesa_Trasferimento_rx % Dopo che i trasduttori sono a regime aspetta di ricevere un messaggio dalla sezione trasmittente
                                  % per avere conferma che è presente e pronta ad effettuare la ricarica della  batteria
                               
        abilita_PWM=0; % Disabilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac
        I_coil_loc_err=0;       % Gli anelli di controllo non sono ancora attivi.
                                % Questo valore al momento non è utilizzato

        Stato_loc=Stato;

        if(Stato_rem==Trasferimento_tx) % Controlla lo stato della sezione ricevente
            Stato=Precarica_Bus_dc_rx;
        end

        if((Stato_rem==Spento_tx)||(Stato_rem==Attesa_Attivazione))
            Stato=Spento_rx;
        end

% Se il trasferimento di potenza viene disabilitato ripristino le condizioni di partenza       
        if(Comando_Attivazione~=Abilita_Ricezione_Potenza)
            Stato=Spento_rx;
        end  



    case Precarica_Bus_dc_rx
        abilita_PWM=0; % Disabilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac

        Stato_loc=Stato;

% La sezione "source" trasmette la potenza alla sezione "load". Il convertitore dc-ac
% della sezione "load" è disabilitato e usato come raddrizzatore. Il riferimento di corrente
% nella bobina "load" è trasmesso via radio alla sezione "source". Si riceve via radio 
% il limite di potenza "Ptrasf_rem_rif_lim" che la sezione "source" può trasmettere .
% Esso viene usato come limite per il riferimento di potenza che si vorrebbe ricevere per
% caricare il condensatore dal bus dc "load". Dal riferimento di potenza si
% calcola il riferimento di corrente e poi l'errore di corrente che viene trasmesso
% via radio alla sezione "source"

        [I_coil_loc_err]=Controllo_Tensione_Bus_dc_I_coil_loc(I_coil_loc,Ptrasf_rem_rif_lim, V_dc, V_dc_Ptrasf_rif, Param_Contr, 1);

% Se ho raggiunto la soglia inferiore per la tensione del bus dc inizio a
% caricare la batteria.
        if(V_dc>=V_dc_ref_low)
            Stato=Trasferimento_rx;
        end

% Se sono arrivato in questo stato significa che precedentemente la sezione "source" era nello stato Trasferimento.
        if(Stato_rem~=Trasferimento_tx)
            Stato=Richiesta_Fine_Trasferimento_rx;
        end

% Se il trasferimento di potenza viene disabilitato ripristino le condizioni di partenza       
        if(Comando_Attivazione~=Abilita_Ricezione_Potenza)
            Stato=Richiesta_Fine_Trasferimento_rx;
        end 




    case Trasferimento_rx
        abilita_PWM=1; % Abilita il convertitore dc-dc
        abilita_PS=0;  % Nella sezione Rx il convertitore dc-ac è sempre disabilitato

        Stato_loc=Stato;

        % Controllo della tensione del bus dc mediante potenza trasferita. Viene calcolato l'errore di corrente nella bobina ricevente            
        [I_coil_loc_err]=Controllo_Tensione_Bus_dc_I_coil_loc(I_coil_loc,Ptrasf_rem_rif_lim, V_dc, V_dc_Ptrasf_rif, Param_Contr, 1);

% Controllo della tensione del bus dc mediante scambio di potenza con la batteria
% Viene generato un riferimento di corrente di batteria
        [I_bat_rif] = Controllo_Tensione_Bus_dc_P_bat(V_dc_Pbat_rif, V_dc, V_bat, I_bat_rif_max, I_bat_rif_min, Param_Contr, 1);         
% Controllo della corrente di batteria            
        [duty_cycle_PWM_A, duty_cycle_PWM_B] = Controllo_Corrente_Batteria(I_bat_rif, I_L_A, I_L_B, V_dc, V_bat, Param_Contr, 1);


        if(abs(I_bat_rif_min)<I_bat_rif_rx_lim) %La batteria si è caricata e non posso più iniettarci potenza  
            Stato=Richiesta_Fine_Trasferimento_rx;
        end

        if(V_dc>V_dc_Ptrasf_rif) %La tensione del bus dc sale troppo. Blocca il trasferimento  
            Stato=Richiesta_Fine_Trasferimento_rx;
        end


% Se sono arrivato in questo stato significa che precedentemente la sezione Tx era nello stato Trasferimento.
        if(Stato_rem~=Trasferimento_tx)
            Stato=Richiesta_Fine_Trasferimento_rx;
        end

% Se il trasferimento di potenza viene disabilitato ripristino le condizioni di partenza       
        if(Comando_Attivazione~=Abilita_Ricezione_Potenza)
            Stato=Richiesta_Fine_Trasferimento_rx;
        end 



    case Richiesta_Fine_Trasferimento_rx % Continua a controllare la tensione del condensatore e la corrente della batteria fino a quando la sezione Tx si spegne
        abilita_PWM=1; % Abilita il convertitore dc-dc
        abilita_PS=0;  % Nella sezione Rx il convertitore dc-ac è sempre disabilitato

        Stato_loc=Stato;

% Controllo della tensione del bus dc mediante potenza trasferita. Viene calcolato l'errore di corrente nella bobina ricevente            
        [I_coil_loc_err]=Controllo_Tensione_Bus_dc_I_coil_loc(I_coil_loc,Ptrasf_rem_rif_lim, V_dc, V_dc_Ptrasf_rif, Param_Contr, 1);

% Controllo della tensione del bus dc mediante scambio di potenza con la batteria
% Viene generato un riferimento di corrente di batteria
        [I_bat_rif] = Controllo_Tensione_Bus_dc_P_bat(V_dc_Pbat_rif, V_dc, V_bat, I_bat_rif_max, I_bat_rif_min, Param_Contr, 1);         
% Controllo della corrente di batteria            
        [duty_cycle_PWM_A, duty_cycle_PWM_B] = Controllo_Corrente_Batteria(I_bat_rif, I_L_A, I_L_B, V_dc, V_bat, Param_Contr, 1);

% % Se la corrente nei rami del convertitore dc-dc sono piccole spengo il sistema            
%         if((abs(I_L_A)<I_L_lim)&&(abs(I_L_B)<I_L_lim))
%             Stato=Spento_rx;
%         end
% Aspetta che la corrente nella bobina "load" si annulli
        if(I_coil_loc<I_coil_loc_min)
            Stato=Spento_tx;
        end


    case Spento_rx
        abilita_PWM=0; % Disbilita il convertitore dc-dc
        abilita_PS=0;  % Disabilita il convertitore dc-ac
        Stato_loc=Stato;
        I_coil_loc_err=0;

% Aspetta che il comando di trasferimento sia disabilitato prima di ripristinare
% le condizioni di partenza
        if(Comando_Attivazione==Annulla_Scambio_Potenza)
            Stato=Attesa_Attivazione;
% Inizializza lo stato della funzioni di controllo
            Controllo_Tensione_Bus_dc_I_coil_loc(0, 0, 0, 0, 0, 0);
            Controllo_Tensione_Bus_dc_P_bat(0, 0, 0, 0, 0, 0, 0);            
            Controllo_Corrente_Batteria(0, 0, 0, 0, 0, 0, 0);
            Contatore=0;
        end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                           Fine anelli di controllo                                 %
%                                  ricevitore                                        %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%      
end


end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                          Funzioni che implementano                                 %
%                           gli anelli di controllo                                  %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 





%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%  Controllo della tensione del bus dc mediante scambio di potenza con la batteria   % 
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [I_bat_rif] = Controllo_Tensione_Bus_dc_P_bat(V_dc_Pbat_rif, V_dc, V_bat, I_bat_rif_max, I_bat_rif_min, Param_Contr, Calcola_1_Inizializza_0)

    persistent Controllo_Tensione_Bus_dc_P_bat_inizializzato
    persistent P_bat_rif_p          % Riferimento di potenza al passo precedente
    persistent V_dc_2_Pbat_err_p    % Errore di tensione al passo precedente

    if(isempty(Controllo_Tensione_Bus_dc_P_bat_inizializzato))
        Controllo_Tensione_Bus_dc_P_bat_inizializzato=1;
         P_bat_rif_p=0;
         V_dc_2_Pbat_err_p=0;
    end

    if(Calcola_1_Inizializza_0==1)

        K_Pbat_rif_p = Param_Contr(4);
        K_V_dc_2_Pbat_err = Param_Contr(5);
        K_V_dc_2_Pbat_err_p = Param_Contr(6);

        % Limiti di potenza trasferibile dalla batteria
        P_bat_rif_max=V_bat * I_bat_rif_max;
        P_bat_rif_min=V_bat * I_bat_rif_min;
    
        % Errore tra riferimento al quadrato di tensione e tensione attuale al quadrato
        V_dc_Pbat_2_err = V_dc_Pbat_rif*V_dc_Pbat_rif - V_dc*V_dc;
    
        % Controllore di tensione. Calcola la potenza da scambiare con la batteria
        P_bat_rif = K_Pbat_rif_p*P_bat_rif_p + K_V_dc_2_Pbat_err*V_dc_Pbat_2_err + K_V_dc_2_Pbat_err_p*V_dc_2_Pbat_err_p;
    
    
        % Limitazione del riferimento di potenza
        if(P_bat_rif>P_bat_rif_max)
            P_bat_rif=P_bat_rif_max;
        end    
    
        if(P_bat_rif<P_bat_rif_min)
            P_bat_rif=P_bat_rif_min;
        end    
    
        % Calcolo del riferimento di corrente di batteria a partire dal riferimento di potenza
        I_bat_rif = P_bat_rif/V_bat;
    
        V_dc_2_Pbat_err_p=V_dc_Pbat_2_err;
        P_bat_rif_p=P_bat_rif;

    else

        I_bat_rif = 0;
        V_dc_2_Pbat_err_p=0;
        P_bat_rif_p=0;    
    
    end
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%                    Controllo della corrente della batteria                         % 
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
function [duty_cycle_PWM_A, duty_cycle_PWM_B] = Controllo_Corrente_Batteria(I_bat_rif, I_L_A, I_L_B, V_dc, V_bat, Param_Contr, Calcola_1_Inizializza_0)

    persistent Controllo_Corrente_Batteria_inizializzato
    persistent I_L_err_A_p
    persistent I_L_err_B_p % Errori di corrente nei due rami del convertitore buck-boost al passo di campionamento precedente
    persistent V_L_rif_A_p
    persistent V_L_rif_B_p % Riferimenti di tensione da applicare alle induttanza di filtro del buck-boost al passo precedente 


    if(isempty(Controllo_Corrente_Batteria_inizializzato))
        Controllo_Corrente_Batteria_inizializzato=1;
        I_L_err_A_p=0;
        I_L_err_B_p=0;
        V_L_rif_A_p=0;
        V_L_rif_B_p=0;
    end

    if(Calcola_1_Inizializza_0==1)

        % Guadagni dei controllori
        K_V_L_rif_p = Param_Contr(1);
        K_I_L_err = Param_Contr(2);
        K_I_L_err_p = Param_Contr(3);

        % Calcolo dei riferimenti per le correnti nei due rami del convertitore buck-boost
        % I due riferimento sono uguali per i due rami 
        I_L_rif_A=0.5*I_bat_rif;
        I_L_rif_B=I_L_rif_A;
        
        % Controllori di corrente
        % Errori di corrente
        I_L_err_A=I_L_rif_A-I_L_A;
        I_L_err_B=I_L_rif_B-I_L_B;
        
        
        % Valori estremi della tensione applicabile sulle induttanze di filtro
        V_L_rif_max=V_bat;      % Corrisponde ad avere il transistor "basso" sempre chiuso
        V_L_rif_min=V_bat-V_dc; % Corrisponde ad avere il transistor "alto" sempre chiuso
        
        % Controllori di corrente. Calcolano la tensione da applicare sulle induttanze di filtro
        V_L_A_rif = K_V_L_rif_p*V_L_rif_A_p + K_I_L_err*I_L_err_A + K_I_L_err_p*I_L_err_A_p;
        V_L_B_rif = K_V_L_rif_p*V_L_rif_B_p + K_I_L_err*I_L_err_B + K_I_L_err_p*I_L_err_B_p;
        
        
        % Limitazione dei riferimenti per evitare windup (il duty cycle non può essere maggiore di 1 o minore di 0)
        if(V_L_A_rif>V_L_rif_max)
            V_L_A_rif=V_L_rif_max;
        end 
        
        if(V_L_A_rif<V_L_rif_min)
            V_L_A_rif=V_L_rif_min;
        end 
        
        if(V_L_B_rif>V_L_rif_max)
            V_L_B_rif=V_L_rif_max;
        end 
        
        if(V_L_B_rif<V_L_rif_min)
            V_L_B_rif=V_L_rif_min;
        end 
        
        % Tensioni da generare all'ingresso (punto di mezzo dei due transistor) del buck-boost
        V_buckboost_A = V_bat - V_L_A_rif;
        V_buckboost_B = V_bat - V_L_B_rif;
        
        % Duty cycle corrispondenti alle tensioni da generare
        duty_cycle_PWM_A = V_buckboost_A / V_dc;
        duty_cycle_PWM_B = V_buckboost_B / V_dc;
    
    
        I_L_err_A_p=I_L_err_A;
        I_L_err_B_p=I_L_err_B;
        V_L_rif_A_p=V_L_A_rif;
        V_L_rif_B_p=V_L_B_rif;

    else

        duty_cycle_PWM_A = 0.5;
        duty_cycle_PWM_B = 0.5;
        I_L_err_A_p=0;
        I_L_err_B_p=0;
        V_L_rif_A_p=0;
        V_L_rif_B_p=0;

    end    

end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                     %
% Controllo della tensione del bus dc mediante potenza trasferita alla sezione remota % 
%                                                                                     %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [P_trasf_rem_rif]=Controllo_Tensione_Bus_dc_P_trasf(V_dc, V_dc_Ptrasf_rif, Icoil_amp_max, Icoil_amp_min, Param_Contr, Calcola_1_Inizializza_0)
    % Controllore PI della tensione del bus dc mediante potenza inviata alla sezione remota


    persistent Controllo_Tensione_Bus_dc_P_trasf_inizializzato
    persistent P_trasf_rem_rif_p  % Riferimento di potenza trasferita di un passo precedente
    persistent V_dc_2_Ptrasf_err_p % Errori di tensione del bus dc del passo precedente e di due passi precedenti

    if(isempty(Controllo_Tensione_Bus_dc_P_trasf_inizializzato))
        Controllo_Tensione_Bus_dc_P_trasf_inizializzato=1;
        P_trasf_rem_rif_p=0;
        V_dc_2_Ptrasf_err_p=0;
    end

    if(Calcola_1_Inizializza_0==1)

        K_P_trasf_rem_rif_p = Param_Contr(15);
        K_V_dc_2_Ptrasf_rem_err = Param_Contr(16);
        K_V_dc_2_Ptrasf_rem_err_p = Param_Contr(17);

        % Limiti di potenza che la sezione locale può trasmettere alla sezione remota
        P_trasf_rem_max=V_dc * Icoil_amp_max * 2/pi;
        P_trasf_rem_min=V_dc * Icoil_amp_min * 2/pi; % ragionevolmente è = 0
    
        % Errore tra riferimento al quadrato di tensione e tensione attuale al quadrato
        V_dc_2_Ptrasf_err = V_dc_Ptrasf_rif*V_dc_Ptrasf_rif - V_dc*V_dc;
        % Devo calcolare il riferimento per la potenza trasferita. Essa scarica il condensatore
        % per cui è positiva quando la tensione del condensatore è maggiore del riferimento (errore < 0)
        % Per non dover usare guadagni negativi cambio il segno dell'errore.
        V_dc_2_Ptrasf_err = -V_dc_2_Ptrasf_err;  
    
        % Controllore di tensione. Calcola la potenza da trasferire sezione remota del sistema WPT
        % per controllare la tensione del bus dc.
        % Questo riferimento di potenza sarà inviato tramite radio alla sezione remota dove 
        % sarà chiamato "Ptrasf_rem_rif_lim"
        P_trasf_rem_rif = K_P_trasf_rem_rif_p*P_trasf_rem_rif_p + K_V_dc_2_Ptrasf_rem_err*V_dc_2_Ptrasf_err + K_V_dc_2_Ptrasf_rem_err_p*V_dc_2_Ptrasf_err_p;
    
        if(P_trasf_rem_rif>P_trasf_rem_max)
            P_trasf_rem_rif=P_trasf_rem_max;
        end
    
        if(P_trasf_rem_rif<P_trasf_rem_min)
            P_trasf_rem_rif=P_trasf_rem_min;
        end
    
        P_trasf_rem_rif_p=P_trasf_rem_rif;
        V_dc_2_Ptrasf_err_p=V_dc_2_Ptrasf_err;

    else

        P_trasf_rem_rif=0;
        P_trasf_rem_rif_p=0;
        V_dc_2_Ptrasf_err_p=0;

    end    

end




function [duty_cycle_PS_A, duty_cycle_PS_B, V_ac_rif]=Controllo_Corrente_Bobina_Remota_e_Limitazione_Corrente_Bobina_Locale...
                                       (V_dc, I_coil_rem_err, I_coil_loc, Param_Contr, Calcola_1_Inizializza_0)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%          Controllo dell'ampiezza della corrente nella bobina ricevente             %
%          mediante tensione generata dal convertitore dc-ac trasmittente            %
%              Limitazione della corrente nella bobina trasmittente                  %
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % Per ottenere i duty-cycle per il convertitore dc-ac è necessario
    % calcolare la funzione
    % 1/pi*arcoseno(V_ac_rif/V_ac_rif_max).
    % A questo scopo è utilizzata una tabella per effettuare una interpolazione lineare.
    %
    % Tabella usata per l'interpolazione della funzione 1/pi*arcoseno() tra 0 e 1
    % La tabella contiene 101 elementi. L'ultimo è usato solo per semplificare il codice.
    % L'indice della tabella in C varia tra 0 e 100. Corrisponde all'argomento dell'arcoseno moltiplicato per 100. 


    tabella_arcsin=[0.000000000000000,   0.003183151915873,   0.006366622213270,   0.009550729560432,   0.012735793199755,   0.015922133236660, ...
            0.019110070930640,   0.022299928989202,   0.025492031865477,   0.028686706060258,   0.031884280429260,   0.035085086496430,...
            0.038289458774147,   0.041497735091205,   0.044710256929508,   0.047927369770437,   0.051149423451922,   0.054376772537300,...
            0.057609776697097,   0.060848801104951,   0.064094216848975,   0.067346401359940,   0.070605738857752,   0.073872620817828,...
            0.077147446459050,   0.080430623255166,   0.083722567471605,   0.087023704729853,   0.090334470601733,   0.093655311236095,...
            0.096986684020678,   0.100329058282131,   0.103682916027458,   0.107048752730465,   0.110427078167105,   0.113818417304015,...
            0.117223311244961,   0.120642318240358,   0.124076014765585,   0.127524996674404,   0.130989880434455,   0.134471304452546,...
            0.137969930498342,   0.141486445235958,   0.145021561874111,   0.148576021946683,   0.152150597236966,   0.155746091860452,...
            0.159363344522883,   0.163003230972354,   0.166666666666667,   0.170354609679922,   0.174068063875524,   0.177808082376468,...
            0.181575771368100,   0.185372294273510,   0.189198876347599,   0.193056809742680,   0.196947459106530,   0.200872267783327,...
            0.204832764699133,   0.208830572026983,   0.212867413742587,   0.216945125200853,   0.221065663886429,   0.225231121519470,...
            0.229443737731699,   0.233705915569418,   0.238020239131091,   0.242389493710302,   0.246816688893365,   0.251305085159287,...
            0.255858224653840,   0.260479966967229,   0.265174530946820,   0.269946543837384,   0.274801099381575,   0.279743826961257,...
            0.284780974456357,   0.289919508299921,   0.295167235300867,   0.300532952314905,   0.306026631958643,   0.311659655572965,...
            0.317445109006172,   0.323398163238602,   0.329536571616801,   0.335881330554762,   0.342457574575956,   0.349295816015145,...
            0.356433706871294,   0.363918619603224,   0.371811566302050,   0.380193416581985,   0.389175313395541,   0.398917375895740,...
            0.409665529398267,   0.421834068069044,   0.436231439141480,   0.454946586355588,   0.500000000000000,   0.454946586355588];



    persistent Controllo_Corrente_Bobina_Remota_inizializzato
    persistent I_coil_rem_err_p
    persistent I_coil_rem_err_pp % Errore di corrente nella bobina all'istante di campionamento precedente
    persistent V_ac_rif_p
    persistent V_ac_rif_pp % Riferimenti di tensione da applicare alla bobina all'istante precedente

    persistent I_coil_loc_err_p
    persistent V_ac_rif_lim_p
    

    if(isempty(Controllo_Corrente_Bobina_Remota_inizializzato))
        Controllo_Corrente_Bobina_Remota_inizializzato=1;
        I_coil_rem_err_p=0;
        I_coil_rem_err_pp=0;
        V_ac_rif_p=0;
        V_ac_rif_pp=0;
        I_coil_loc_err_p=0;
        V_ac_rif_lim_p=0;
    end

    if(Calcola_1_Inizializza_0==1)

        I_coil_loc_lim=50;      % Ampiezza massima della corrente nella bobina trasmittente

        K_V_coil_rif_p = Param_Contr(7);
        K_V_coil_rif_pp = Param_Contr(8);
        K_I_coil_err = Param_Contr(9);
        K_I_coil_err_p = Param_Contr(10);
        K_I_coil_err_pp = Param_Contr(11);

        K_V_coil_lim_p = Param_Contr(18);
        K_I_coil_loc_err = Param_Contr(19);
        K_I_coil_loc_err_p = Param_Contr(20);

    
        % Massima tensione ac che può essere generata
        V_ac_rif_max = V_dc*4/pi;
        
        % Riferimento di tensione
        V_ac_rif = K_V_coil_rif_p*V_ac_rif_p + K_V_coil_rif_pp*V_ac_rif_pp + K_I_coil_err*I_coil_rem_err +...
                             K_I_coil_err_p*I_coil_rem_err_p + K_I_coil_err_pp*I_coil_rem_err_pp;
    
        % Limitazione del riferimento di tensione
        if(V_ac_rif>V_ac_rif_max) % La prima armonica di tensione generata dal convertitore dc-ac non può essere maggiore di 4/pi * Vdc
            V_ac_rif=V_ac_rif_max;
        end 
        
        % if(V_ac_rif<0)
        %      V_ac_rif=0;
        % end 

       if(V_ac_rif<-V_ac_rif_max)
           V_ac_rif=-V_ac_rif_max;
       end 
    
        % Calcolo del limite riferimento di tensione
        I_coil_loc_err=I_coil_loc_lim-I_coil_loc;
        % Riferimento limite di tensione
        V_ac_rif_lim = K_V_coil_lim_p*V_ac_rif_lim_p + K_I_coil_loc_err*I_coil_loc_err + K_I_coil_loc_err_p*I_coil_loc_err_p;
    
    
        % Limitazione del riferimento di tensione
        if(V_ac_rif_lim>V_ac_rif_max) % La prima armonica di tensione generata dal convertitore dc-ac non può essere maggiore di 4/pi * Vdc
            V_ac_rif_lim=V_ac_rif_max;
        end 
        
        if(V_ac_rif_lim<0)
            V_ac_rif_lim=0;
        end
     
        % if(V_ac_rif>V_ac_rif_lim)
        %     V_ac_rif=V_ac_rif_lim;
        % end    
        % 
        % if(V_ac_rif_lim>(V_ac_rif+1))
        %     V_ac_rif_lim=V_ac_rif+1;
        % end    
    
        if(abs(V_ac_rif)>V_ac_rif_lim)
            V_ac_rif=sign(V_ac_rif)*V_ac_rif_lim;
        end    

        if(V_ac_rif_lim>(abs(V_ac_rif)+1))
            V_ac_rif_lim=abs(V_ac_rif)+1;
        end

   
%        arg_arcsin=V_ac_rif/V_ac_rif_max; % argomento della funzione arcoseno
        
        arg_arcsin=abs(V_ac_rif)/V_ac_rif_max; % argomento della funzione arcoseno

        
        if(arg_arcsin>1)
            arg_arcsin=1;
        end    
        
        % interpolazione della funzione arcoseno
        indice_argomento=arg_arcsin*100; % indice ella tabella 
        indice_argomento_prec=floor(indice_argomento); % corrisponde a (int)(indice argomento) E' l'intero immediatamente inferiore all'argomento dell'arcoseno*100. 
        indice_argomento_suc=indice_argomento_prec+1; % Intero immediatamente superiore all'argomento dell'arcoseno*100.
        % il valore "corretto" dell'arcoseno di arg_arcsin è compreso tra i due
        % elementi della tabela che si trovano nelle posizioni indice_argomento_prec e indice_argomento_suc
        duty_prec=tabella_arcsin(indice_argomento_prec+1); 
        duty_suc=tabella_arcsin(indice_argomento_suc+1);
        
        % Approssimo il valore corretto mediante una interpolazione lineare
        delta_indice_argomento=indice_argomento-indice_argomento_prec; 
        duty=duty_prec+delta_indice_argomento*(duty_suc-duty_prec);
        
        if(V_ac_rif<0)
            duty=-duty;
        end    


        duty_cycle_PS_A=0.5+duty;
        duty_cycle_PS_B=0.5-duty;
    
        V_ac_rif_pp=V_ac_rif_p;
        V_ac_rif_p=V_ac_rif;
        I_coil_rem_err_pp=I_coil_rem_err_p;
        I_coil_rem_err_p=I_coil_rem_err;
    
    
        V_ac_rif_lim_p=V_ac_rif_lim;
        I_coil_loc_err_p=I_coil_loc_err;
    else

        duty_cycle_PS_A=0.5;
        duty_cycle_PS_B=0.5;
        V_ac_rif_pp=0;
        V_ac_rif_p=0;
        I_coil_rem_err_pp=0;
        I_coil_rem_err_p=0;
        V_ac_rif_lim_p=0;
        I_coil_loc_err_p=0;

    end    
end




function [I_coil_loc_err]=Controllo_Tensione_Bus_dc_I_coil_loc(I_coil_loc,Ptrasf_rem_rif_lim, V_dc, V_dc_Ptrasf_rif, Param_Contr, Calcola_1_Inizializza_0)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                                    %
%      Controllo della tensione del bus dc mediante corrente nella bobina locale     % 
%                                                                                    %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% La sezione remota trasmette la potenza alla sezione locale. Il convertitore dc-ac
% locale è disattivato e usato come raddrizzatore. Il riferimento di corrente
% nella bobina locale è trasmesso via radio alla sezione remota. Si riceve via radio 
% il limite di potenza che la sezione remota può trasmettere "Ptrasf_rem_rif_lim".
% Esso viene usato come limite per il riferimento di potenza che si vorrebbe ricevere per
% caricare il condensatore dal bus dc locale. Dal riferimento di potenza si
% calcola il riferimento di corrente e poi l'errore di corrente che viene trasmesso
% via radio alla sezione remota

    
    persistent Controllo_Tensione_Bus_dc_I_coil_loc_inizializzato
    persistent V_dc_2_Ptrasf_err_p % Errori di tensione del bus dc del passo precedente e di due passi precedenti
    persistent P_trasf_loc_rif_p  % Riferimento di potenza trasferita di un passo precedente e di due passi precedenti 

    if(isempty(Controllo_Tensione_Bus_dc_I_coil_loc_inizializzato))
        Controllo_Tensione_Bus_dc_I_coil_loc_inizializzato=1;
        V_dc_2_Ptrasf_err_p=0;
        P_trasf_loc_rif_p=0;
    end

    if(Calcola_1_Inizializza_0==1)
    
        K_P_trasf_loc_rif_p = Param_Contr(12);
        K_V_dc_2_Ptrasf_loc_err = Param_Contr(13);
        K_V_dc_2_Ptrasf_loc_err_p = Param_Contr(14);

    % Limiti di potenza che la sezione remota può trasmettere alla sezione locale
        P_trasf_loc_max=Ptrasf_rem_rif_lim;
        P_trasf_loc_min=0;
        
        % Errore tra riferimento al quadrato di tensione e tensione attuale al quadrato
        V_dc_2_Ptrasf_err = V_dc_Ptrasf_rif*V_dc_Ptrasf_rif - V_dc*V_dc;
        
        % Controllore di tensione. Calcola la potenza da iniettare nel condensatore del bus dc
        P_trasf_loc_rif = K_P_trasf_loc_rif_p*P_trasf_loc_rif_p + K_V_dc_2_Ptrasf_loc_err*V_dc_2_Ptrasf_err + K_V_dc_2_Ptrasf_loc_err_p*V_dc_2_Ptrasf_err_p;
        
        % Limitazione del riferimento di potenza
        if(P_trasf_loc_rif>P_trasf_loc_max)
            P_trasf_loc_rif=P_trasf_loc_max;
        end    
        
        if(P_trasf_loc_rif<P_trasf_loc_min)
            P_trasf_loc_rif=P_trasf_loc_min;
        end    
        
        % Il riferimento di corrente è poi trasmesso alla sezione remota dove è
        % implementato il controllore
        
        % Calcolo del riferimento di ampiezza per la corrente nella bobina locale
        I_coil_loc_rif = pi/2 * P_trasf_loc_rif/V_dc;
        
        % Errore di ampiezza della corrente locale
        % Questa grandezza va trasmessa via radio all'altra sezione del sistema
        I_coil_loc_err=I_coil_loc_rif-I_coil_loc;
    
        V_dc_2_Ptrasf_err_p=V_dc_2_Ptrasf_err;
    
        P_trasf_loc_rif_p=P_trasf_loc_rif;

    else

        I_coil_loc_err=0;
        V_dc_2_Ptrasf_err_p=0;
        P_trasf_loc_rif_p=0;

    end    
end
